#include <teximp/exr/exr_importer.openexr.h>

#ifdef TEXIMP_ENABLE_EXR_BACKEND_OPENEXR

#ifdef TEXIMP_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable :4244) // C4244 : '=' : conversion from 'int' to 'unsigned short', possible loss of data
#pragma warning(disable :4100) // C4100 : 'version' : unreferenced formal parameter
#endif

#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfChannelListAttribute.h>
#include <OpenEXR/ImfCompressionAttribute.h>
#include <OpenEXR/ImfFloatVectorAttribute.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfInputPart.h>
#include <OpenEXR/ImfIntAttribute.h>
#include <OpenEXR/ImfIO.h>
#include <OpenEXR/ImfLineOrderAttribute.h>
#include <OpenEXR/ImfMultiPartInputFile.h>
#include <OpenEXR/ImfPreviewImageAttribute.h>
#include <OpenEXR/ImfStandardAttributes.h>
#include <OpenEXR/ImfTestFile.h>
#include <OpenEXR/ImfTileDescriptionAttribute.h>
#include <OpenEXR/ImfTiledInputFile.h>
#include <OpenEXR/ImfVersion.h>

#ifdef TEXIMP_COMPILER_MSVC
#pragma warning(pop)
#endif

#include "utilities.h"

#include <cputex/utility.h>
#include <errno.h>

#include <bitset>
#include <fstream>
#include <optional>
#include <sstream>

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_ENTER
class StdIStream final : public Imf::IStream
{
public:
    StdIStream(const char fileName[])
        : Imf::IStream(fileName)
        , mFileStream(fileName, std::ios_base::in | std::ios_base::binary)
    {
        if(mFileStream.is_open()) { mStream = &mFileStream; }
    }

    StdIStream(std::istream& stream, const char fileName[])
        : Imf::IStream(fileName)
        , mStream(&stream)
    {}

    ~StdIStream() final = default;

    bool read(char c[], int n) final
    {
        if(mStream == nullptr) { return false; }

        clearError();
        mStream->read(c, n);
        return checkError(*mStream, n);
    }

    uint64_t tellg() final
    {
        if(mStream == nullptr) { return 0; }

        return std::streamoff(mStream->tellg());
    }

    void seekg(uint64_t pos) final
    {
        if(mStream == nullptr) { return; }

        mStream->seekg(pos);
    }

    void clear() final
    {
        if(mStream == nullptr) { return; }

        mStream->clear();
    }

private:
    void clearError() { errno = 0; }

    bool checkError(std::istream& is, [[maybe_unused]] std::streamsize expected = 0)
    {
        if(!is) { return false; }

        return true;
    }

    std::ifstream mFileStream;
    std::istream* mStream = nullptr;
};

class StdStringStream final : public OStream
{
public:
    StdStringStream()
        : OStream("")
        , mStream(std::ios_base::out)
    {}

    ~StdStringStream() final = default;

    void write(const char c[/*n*/], int n) final { mStream.write(c, n); }

    uint64_t tellp() final { return mStream.tellp(); }

    void seekp(uint64_t pos) final { mStream.seekp(pos); }

    std::string toString() const { return mStream.str(); }

private:
    std::stringstream mStream;
};

OPENEXR_IMF_INTERNAL_NAMESPACE_SOURCE_EXIT

namespace teximp::exr
{
struct ChannelPtrPair
{
    std::string name;
    const Imf::Channel* channel = nullptr;
};

std::vector<ExrOpenExrImporter::SubViewLayout>
assignChannelsToLayouts(const Imf::ChannelList& channelList, const std::string& layerName,
                        std::vector<ExrOpenExrImporter::SubViewLayout>& layouts)
{
    static constexpr std::array<std::string_view, 4> colorNames = {"r", "g", "b", "a"};
    static constexpr std::array<std::string_view, 3> positionNames = {"x", "y", "z"};
    static constexpr std::array<std::string_view, 3> uvNames = {"u", "v", "w"};

    std::array<ExrOpenExrImporter::SubViewLayout, 3> colorLayouts;
    std::array<ExrOpenExrImporter::SubViewLayout, 3> positionLayouts;
    std::array<ExrOpenExrImporter::SubViewLayout, 3> uvLayouts;
    std::vector<ExrOpenExrImporter::SubViewLayout> unknownChannelLayouts;

    static_assert(colorLayouts.size() == Imf::PixelType::NUM_PIXELTYPES);

    // fill the layouts with the appropriate Imf::PixelType
    auto assignPixelType = [](std::array<ExrOpenExrImporter::SubViewLayout, 3>& layouts)
    {
        for(int i = 0; i < layouts.size(); ++i)
        {
            layouts[i].pixelType = static_cast<Imf::PixelType>(i);
        }
    };

    assignPixelType(colorLayouts);
    assignPixelType(positionLayouts);
    assignPixelType(uvLayouts);

    // find the begin and end of the layer. if the layer
    // is and empty string, find all the layers without a '.'.
    Imf::ChannelList::ConstIterator layerBegin, layerEnd;
    if(!layerName.empty()) { channelList.channelsInLayer(layerName, layerBegin, layerEnd); }
    else
    {
        layerBegin = channelList.begin();
        layerEnd = channelList.end();

        for(auto itr = channelList.begin(); itr != channelList.end(); ++itr)
        {
            if(std::strchr(itr.name(), '.') != nullptr)
            {
                layerEnd = itr;
                break;
            }
        }
    }

    auto resolveChannelName = [](std::string_view fullChannelName, std::string_view channelName,
                                 const Imf::Channel& channel, std::span<const std::string_view> knownNames,
                                 ExrOpenExrImporter::SubViewLayout& layout) -> bool
    {
        auto findItr = std::find_if(knownNames.begin(), knownNames.end(),
                                    [channelName](const std::string_view& knownName)
                                    { return caseInsensitiveEqual(channelName, knownName); });

        if(findItr != knownNames.end())
        {
            size_t index = std::distance(knownNames.begin(), findItr);

            layout.channels[index].name = fullChannelName;
            layout.channels[index].channel = channel;
            layout.channelMask |= (gpufmt::ChannelMask)((size_t)gpufmt::ChannelMask::Red << index);
            return true;
        }

        return false;
    };

    for(auto channelItr = layerBegin; channelItr != layerEnd; ++channelItr)
    {
        std::string_view nameView{channelItr.name(), std::strlen(channelItr.name())};

        std::string_view channelName = nameView.substr((!layerName.empty()) ? layerName.size() + 1 : 0);

        // colors
        if(resolveChannelName(nameView, channelName, channelItr.channel(), colorNames,
                              colorLayouts[channelItr.channel().type]))
        {
            continue;
        }

        // positions
        if(resolveChannelName(nameView, channelName, channelItr.channel(), positionNames,
                              positionLayouts[channelItr.channel().type]))
        {
            continue;
        }

        // uvs
        if(resolveChannelName(nameView, channelName, channelItr.channel(), uvNames,
                              uvLayouts[channelItr.channel().type]))
        {
            continue;
        }

        ExrOpenExrImporter::SubViewLayout& unknownLayout = unknownChannelLayouts.emplace_back();
        unknownLayout.channelMask = gpufmt::ChannelMask::Red;
        unknownLayout.channels[0].name = nameView;
        unknownLayout.channels[0].channel = channelItr.channel();
        unknownLayout.pixelType = channelItr.channel().type;
    }

    layouts.reserve(layouts.size() + unknownChannelLayouts.size() + 3);

    for(ExrOpenExrImporter::SubViewLayout& layout : colorLayouts)
    {
        if(layout.channelMask != gpufmt::ChannelMask::None) { layouts.emplace_back(std::move(layout)); }
    }

    for(ExrOpenExrImporter::SubViewLayout& layout : positionLayouts)
    {
        if(layout.channelMask != gpufmt::ChannelMask::None)
        {
            if(layout.channelMask == gpufmt::ChannelMask::Blue)
            {
                // fix the case where we only found a z value
                std::swap(layout.channels[0], layout.channels[2]);
                layout.channelMask = gpufmt::ChannelMask::Red;
            }
            else if(layout.channelMask == gpufmt::ChannelMask::Green)
            {
                // fix the case where we only found a y value
                std::swap(layout.channels[0], layout.channels[1]);
                layout.channelMask = gpufmt::ChannelMask::Red;
            }

            layouts.emplace_back(std::move(layout));
        }
    }

    for(ExrOpenExrImporter::SubViewLayout& layout : uvLayouts)
    {
        if(layout.channelMask != gpufmt::ChannelMask::None) { layouts.emplace_back(std::move(layout)); }
    }

    std::move(unknownChannelLayouts.begin(), unknownChannelLayouts.end(), std::back_inserter(layouts));

    return layouts;
}

gpufmt::Format getLayoutFormat(const ExrOpenExrImporter::SubViewLayout& layout)
{
    if(layout.pixelType == Imf::PixelType::HALF)
    {
        if(enumMaskTest(layout.channelMask, gpufmt::ChannelMask::Alpha)) { return gpufmt::Format::R16G16B16A16_SFLOAT; }
        else if(enumMaskTest(layout.channelMask, gpufmt::ChannelMask::Blue))
        {
            return gpufmt::Format::R16G16B16_SFLOAT;
        }
        else if(enumMaskTest(layout.channelMask, gpufmt::ChannelMask::Green)) { return gpufmt::Format::R16G16_SFLOAT; }
        else { return gpufmt::Format::R16_SFLOAT; }
    }
    else if(layout.pixelType == Imf::PixelType::FLOAT)
    {
        if(enumMaskTest(layout.channelMask, gpufmt::ChannelMask::Alpha)) { return gpufmt::Format::R32G32B32A32_SFLOAT; }
        else if(enumMaskTest(layout.channelMask, gpufmt::ChannelMask::Blue))
        {
            return gpufmt::Format::R32G32B32_SFLOAT;
        }
        else if(enumMaskTest(layout.channelMask, gpufmt::ChannelMask::Green)) { return gpufmt::Format::R32G32_SFLOAT; }
        else { return gpufmt::Format::R32_SFLOAT; }
    }
    else if(layout.pixelType == Imf::PixelType::UINT)
    {
        if(enumMaskTest(layout.channelMask, gpufmt::ChannelMask::Alpha)) { return gpufmt::Format::R32G32B32A32_UINT; }
        else if(enumMaskTest(layout.channelMask, gpufmt::ChannelMask::Blue)) { return gpufmt::Format::R32G32B32_UINT; }
        else if(enumMaskTest(layout.channelMask, gpufmt::ChannelMask::Green)) { return gpufmt::Format::R32G32_UINT; }
        else { return gpufmt::Format::R32_UINT; }
    }

    return gpufmt::Format::UNDEFINED;
}

int fillFrameBuffer(Imf::FrameBuffer& frameBuffer, glm::ivec2 min, const ExrOpenExrImporter::SubViewLayout& layout,
                    const cputex::Extent& textureExtent, std::span<std::byte> textureData)
{
    const gpufmt::FormatInfo& formatInfo = gpufmt::formatInfo(getLayoutFormat(layout));
    const uint32_t channelByteSize = formatInfo.blockByteSize / formatInfo.componentCount;
    int frameBufferCount = 0;
    uint32_t channelOffset = 0u;

    size_t xStride = static_cast<size_t>(formatInfo.blockByteSize);
    size_t yStride = xStride * textureExtent.x;

    char* pixelBase = castWritableBytes<char>(textureData).data() - min.x * xStride - min.y * yStride;

    for(uint32_t i = 0; i < layout.channels.size(); ++i)
    {
        auto channel = static_cast<gpufmt::ChannelMask>(static_cast<uint32_t>(gpufmt::ChannelMask::Red) << i);

        if(enumMaskTest(layout.channelMask, channel))
        {
            frameBuffer.insert(layout.channels[i].name,
                               Imf::Slice{
                                   layout.pixelType,                     // type
                                   pixelBase + channelOffset,            // base
                                   xStride,                              // xStride
                                   yStride,                              // yStride
                                   layout.channels[i].channel.xSampling, // x sampling
                                   layout.channels[i].channel.ySampling, // y sampling
                                   0.0f                                  // fillValue
                               });
            ++frameBufferCount;
        }

        channelOffset += channelByteSize;
    }

    return frameBufferCount;
}

void splitChannelName(std::string_view channelName, std::span<const std::string> viewNames, std::string_view& layer,
                      std::string_view& view, std::string_view& channel)
{
    auto lastPeriodItr = std::find(channelName.crbegin(), channelName.crend(), '.');

    if(lastPeriodItr == channelName.crend())
    {
        // no periods were found and its just a channel name
        layer = {};
        view = {};
        channel = channelName;
        return;
    }

    auto secondToLastPeriodItr = std::find(lastPeriodItr + 1, channelName.crend(), '.');

    size_t channelOffset = std::distance(lastPeriodItr, channelName.crend());
    channel = channelName.substr(channelOffset);

    if(secondToLastPeriodItr == channelName.crend())
    {
        // it's either a layer or view name

        // check views
        for(const std::string& currentViewName : viewNames)
        {
            auto test = lastPeriodItr.base();
            bool isView = std::equal(channelName.cbegin(), lastPeriodItr.base() - 1, currentViewName.cbegin(),
                                     currentViewName.cend());

            if(isView)
            {
                layer = {};
                view = currentViewName;
                return;
            }
        }

        // it's a layer
        layer = channelName.substr(0, std::distance(lastPeriodItr, channelName.crend()));
        view = {};
    }
    else
    {
        size_t layerSize = std::distance(secondToLastPeriodItr, channelName.crend());
        size_t viewOffset = layerSize;
        size_t viewSize = std::distance(lastPeriodItr, secondToLastPeriodItr);

        layer = channelName.substr(0, layerSize);
        view = channelName.substr(viewOffset, viewSize);
    }
}

std::map<std::string, ExrOpenExrImporter::AttributeVariant> parseAttributes(const Imf::Header& header)
{
    using namespace std::literals;

    std::map<std::string, ExrOpenExrImporter::AttributeVariant> attributes;

    for(auto itr = header.begin(); itr != header.end(); ++itr)
    {
        const Imf::Attribute& attr = itr.attribute();

        std::string_view typeName{attr.typeName(), std::strlen(attr.typeName())};

        if(caseInsensitiveEqual(typeName, Imf::FloatAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::FloatAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::FloatVectorAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::FloatVectorAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::IntAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::IntAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::StringAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::StringAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::StringVectorAttribute::staticTypeName()))
        {
            attributes.emplace(
                std::make_pair(itr.name(), static_cast<const Imf::StringVectorAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::CompressionAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::CompressionAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::DeepImageStateAttribute::staticTypeName()))
        {
            attributes.emplace(
                std::make_pair(itr.name(), static_cast<const Imf::DeepImageStateAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::EnvmapAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::EnvmapAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::LineOrderAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::LineOrderAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::ChannelListAttribute::staticTypeName()))
        {
            int channelCount = 0;
            for(auto channelItr = header.channels().begin(); channelItr != header.channels().end(); ++channelItr)
            {
                ++channelCount;
            }

            attributes.emplace(std::make_pair(itr.name(), ExrOpenExrImporter::ChannelList{channelCount}));
        }
        else if(caseInsensitiveEqual(typeName, Imf::ChromaticitiesAttribute::staticTypeName()))
        {
            attributes.emplace(
                std::make_pair(itr.name(), static_cast<const Imf::ChromaticitiesAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::KeyCodeAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::KeyCodeAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::PreviewImageAttribute::staticTypeName()))
        {
            ExrOpenExrImporter::PreviewImage previewImage{header.previewImage().width(),
                                                          header.previewImage().height()};
            attributes.emplace(std::make_pair(itr.name(), previewImage));
        }
        else if(caseInsensitiveEqual(typeName, Imf::RationalAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::RationalAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::TileDescriptionAttribute::staticTypeName()))
        {
            attributes.emplace(
                std::make_pair(itr.name(), static_cast<const Imf::TileDescriptionAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::TimeCodeAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::TimeCodeAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::Box2fAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::Box2fAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::Box2iAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::Box2iAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::M33dAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::M33dAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::M33fAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::M33fAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::M44dAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::M44dAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::M44fAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::M44fAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::V2dAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::V2dAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::V2fAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::V2fAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::V2iAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::V2iAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::V3dAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::V3dAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::V3fAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::V3fAttribute&>(attr).value()));
        }
        else if(caseInsensitiveEqual(typeName, Imf::V3iAttribute::staticTypeName()))
        {
            attributes.emplace(std::make_pair(itr.name(), static_cast<const Imf::V3iAttribute&>(attr).value()));
        }
        else
        {
            Imf::StdStringStream stream;
            attr.writeValueTo(stream, Imf::getVersion(header.version()));

            attributes.emplace(std::make_pair(
                itr.name(), ExrOpenExrImporter::UnknownAttributeType{attr.typeName(), stream.toString()}));
        }
    }

    return attributes;
}

FileFormat ExrOpenExrImporter::fileFormat() const
{
    return FileFormat::Exr;
}

bool ExrOpenExrImporter::checkSignature(std::istream& stream)
{
    std::string filePathStr = filePath().string();
    Imf::StdIStream exrStream{stream, filePathStr.c_str()};

    return Imf::isOpenExrFile(exrStream);
}

void ExrOpenExrImporter::load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options)
{
    std::string filePathStr = filePath().string();
    Imf::StdIStream exrStream{stream, filePathStr.c_str()};

    bool isTiled = Imf::isTiledOpenExrFile(exrStream);
    bool isMultiPart = Imf::isMultiPartOpenExrFile(exrStream);
    bool isDeep = Imf::isDeepOpenExrFile(exrStream);

    if(isMultiPart) { loadMultiPartImage(exrStream, textureAllocator, options); }
    else if(isTiled) { loadTiledImage(exrStream, textureAllocator, options); }
    else if(isDeep) { loadDeepImage(exrStream, textureAllocator, options); }
    else { loadImage(exrStream, textureAllocator, options); }
}

void ExrOpenExrImporter::loadDeepImage(Imf::StdIStream&, ITextureAllocator&, TextureImportOptions)
{
    setError(TextureImportError::UnsupportedFeature, "EXR deep images are not currently supported");
}

void ExrOpenExrImporter::loadImage(Imf::StdIStream& exrStream, ITextureAllocator& textureAllocator,
                                   TextureImportOptions /*options*/)
{
    Imf::InputFile inputFile{exrStream};

    mParts.reserve(1);
    Part& part = mParts.emplace_back(Part{inputFile.header()});
    const Imf::ChannelList& channelList = part.header.channels();
    channelList.layers(part.layerNames);

    if(Imf::hasMultiView(inputFile.header()))
    {
        part.viewNames = Imf::multiView(inputFile.header());

        part.views.reserve(part.viewNames.size());

        for(const std::string viewName : part.viewNames)
        {
            part.views.emplace_back(View{viewName});
        }

        part.views.emplace_back();
        part.viewNames.emplace_back();
    }
    else
    {
        View& view = part.views.emplace_back();

        if(part.header.hasView())
        {
            view.name = part.header.view();
            part.viewNames.emplace_back(view.name);
        }
        else { part.viewNames.emplace_back(); }
    }

    Properties properties;
    Imath::Box2i dataWindow = inputFile.header().dataWindow();
    properties.dimensions.x = dataWindow.max.x - dataWindow.min.x + 1;
    properties.dimensions.y = dataWindow.max.y - dataWindow.min.y + 1;
    properties.drawDataMin = {dataWindow.min.x, dataWindow.min.y};

    Imf::FrameBuffer frameBuffer;
    int frameBufferCount = 0;

    frameBufferCount = extractAllLayouts(properties, part, std::span(&frameBuffer, 1), textureAllocator);

    for(const View& view : part.views)
    {
        part.totalSubViews += static_cast<int>(view.subViewLayouts.size());
    }

    if(frameBufferCount == 0)
    {
        setError(TextureImportError::UnsupportedFeature, "Currently only R, G, B, and A channels are supported.");
        return;
    }

    inputFile.setFrameBuffer(frameBuffer);
    inputFile.readPixels(dataWindow.min.y, dataWindow.max.y);

    part.attributes = parseAttributes(inputFile.header());
}

void ExrOpenExrImporter::loadMultiPartImage(Imf::StdIStream& exrStream, ITextureAllocator& textureAllocator,
                                            TextureImportOptions /*options*/)
{
    Imf::MultiPartInputFile multiPartInputFile{exrStream};
    int partsCount = multiPartInputFile.parts();
    mParts.reserve(partsCount);

    for(int i = 0; i < partsCount; ++i)
    {
        Imf::InputPart inputFilePart{multiPartInputFile, i};

        Part& part = mParts.emplace_back(Part{inputFilePart.header()});
        part.header.channels().layers(part.layerNames);

        if(Imf::hasMultiView(inputFilePart.header()))
        {
            part.viewNames = Imf::multiView(inputFilePart.header());

            part.views.reserve(part.viewNames.size());

            for(const std::string viewName : part.viewNames)
            {
                part.views.emplace_back(View{viewName});
            }

            part.views.emplace_back();
            part.viewNames.emplace_back();
        }
        else
        {
            View& view = part.views.emplace_back();

            if(part.header.hasView())
            {
                part.viewNames.emplace_back(part.header.view());
                view.name = part.header.view();
            }
            else { part.viewNames.emplace_back(); }
        }

        Properties properties;
        Imath::Box2i dataWindow = inputFilePart.header().dataWindow();
        properties.dimensions.x = dataWindow.max.x - dataWindow.min.x + 1;
        properties.dimensions.y = dataWindow.max.y - dataWindow.min.y + 1;
        properties.drawDataMin = {dataWindow.min.x, dataWindow.min.y};

        Imf::FrameBuffer frameBuffer;
        int frameBufferCount = 0;

        frameBufferCount = extractAllLayouts(properties, part, std::span(&frameBuffer, 1), textureAllocator);

        for(const View& view : part.views)
        {
            part.totalSubViews += static_cast<int>(view.subViewLayouts.size());
        }

        if(frameBufferCount == 0)
        {
            setError(TextureImportError::UnsupportedFeature, "Currently only R, G, B, and A channels are supported.");
            return;
        }

        inputFilePart.setFrameBuffer(frameBuffer);
        inputFilePart.readPixels(dataWindow.min.y, dataWindow.max.y);

        part.attributes = parseAttributes(inputFilePart.header());
    }
}

void ExrOpenExrImporter::loadTiledImage(Imf::StdIStream& exrStream, ITextureAllocator& textureAllocator,
                                        TextureImportOptions /*options*/)
{
    Imf::TiledInputFile inputFile{exrStream};

    if(inputFile.levelMode() == Imf::LevelMode::RIPMAP_LEVELS)
    {
        setError(TextureImportError::UnsupportedFeature, "EXR tiled images with ripmaps are not currently supported");
        return;
    }

    if(inputFile.levelRoundingMode() == Imf::LevelRoundingMode::ROUND_UP)
    {
        setError(TextureImportError::UnsupportedFeature,
                 "EXR tiled images with level rounding mode 'round up' are not currently supported");
        return;
    }

    mParts.reserve(1);
    Part& part = mParts.emplace_back(Part{inputFile.header()});
    const Imf::ChannelList& channelList = part.header.channels();
    channelList.layers(part.layerNames);

    if(Imf::hasMultiView(inputFile.header()))
    {
        part.viewNames = Imf::multiView(inputFile.header());

        part.views.reserve(part.viewNames.size());

        for(const std::string viewName : part.viewNames)
        {
            part.views.emplace_back(View{viewName});
        }

        part.views.emplace_back();
        part.viewNames.emplace_back();
    }
    else
    {
        part.views.emplace_back();
        part.viewNames.emplace_back();
    }

    Properties properties;
    Imath::Box2i dataWindow = inputFile.header().dataWindow();
    properties.dimensions.x = dataWindow.max.x - dataWindow.min.x + 1;
    properties.dimensions.y = dataWindow.max.y - dataWindow.min.y + 1;
    properties.drawDataMin = {dataWindow.min.x, dataWindow.min.y};
    properties.mips = inputFile.numLevels();

    std::vector<Imf::FrameBuffer> frameBuffers(properties.mips);
    int frameBufferCount = 0;

    frameBufferCount = extractAllLayouts(properties, part, frameBuffers, textureAllocator);

    for(const View& view : part.views)
    {
        part.totalSubViews += static_cast<int>(view.subViewLayouts.size());
    }

    if(frameBufferCount == 0)
    {
        setError(TextureImportError::UnsupportedFeature, "Currently only R, G, B, and A channels are supported.");
        return;
    }

    for(int mip = 0; mip < properties.mips; ++mip)
    {
        int xTiles = inputFile.numXTiles(mip);
        int yTiles = inputFile.numYTiles(mip);

        inputFile.setFrameBuffer(frameBuffers[mip]);
        inputFile.readTiles(0, xTiles - 1, 0, yTiles - 1, mip);
    }

    part.attributes = parseAttributes(inputFile.header());
}

bool ExrOpenExrImporter::createTextureForLayout(ITextureAllocator& textureAllocator, int textureIndex,
                                                ExrOpenExrImporter::SubViewLayout& layout, int width, int height,
                                                int mips)
{
    cputex::TextureParams params;
    params.arraySize = 1;
    params.dimension = cputex::TextureDimension::Texture2D;
    params.extent = cputex::Extent{width, height, 1};
    params.faces = 1;
    params.mips = mips;
    params.surfaceByteAlignment = 4;
    params.format = getLayoutFormat(layout);

    if(!textureAllocator.allocateTexture(params, textureIndex))
    {
        setTextureAllocationError(params);
        return false;
    }

    layout.textureIndex = textureIndex;
    return true;
}

int ExrOpenExrImporter::extractAllLayouts(const Properties& properties, Part& part,
                                          std::span<Imf::FrameBuffer> frameBuffers, ITextureAllocator& textureAllocator)
{
    int frameBufferCount = 0;

    static const std::string emptyString;
    assignChannelsToLayouts(part.header.channels(), emptyString, part.views.front().subViewLayouts);

    for(const std::string& layerName : part.layerNames)
    {
        auto lastPeriodRevItr = std::find(layerName.crbegin(), layerName.crend(), '.');

        std::string_view viewName;

        if(lastPeriodRevItr == layerName.crend())
        {
            // its just a single layer
            auto findItr = std::find(part.viewNames.cbegin(), part.viewNames.cend(), layerName);

            if(findItr != part.viewNames.cend()) { viewName = *findItr; }
        }
        else
        {
            size_t offset = std::distance(lastPeriodRevItr, layerName.crend());
            size_t size = std::distance(layerName.crbegin(), lastPeriodRevItr);
            std::string_view lastLayerName{layerName.data() + offset, size};

            auto findItr = std::find(part.viewNames.cbegin(), part.viewNames.cend(), lastLayerName);

            if(findItr != part.viewNames.cend()) { viewName = *findItr; }
        }

        auto findItr = std::find_if(part.views.begin(), part.views.end(),
                                    [&viewName](View& view) { return viewName == view.name; });

        if(findItr == part.views.end())
        {
            assignChannelsToLayouts(part.header.channels(), layerName, part.views.front().subViewLayouts);
        }
        else { assignChannelsToLayouts(part.header.channels(), layerName, findItr->subViewLayouts); }
    }

    // remove all empty views
    auto viewItr = part.views.begin();
    auto viewNameItr = part.viewNames.begin();

    while(viewItr != part.views.end())
    {
        if(viewItr->subViewLayouts.empty())
        {
            viewItr = part.views.erase(viewItr);
            viewNameItr = part.viewNames.erase(viewNameItr);
        }
        else
        {
            ++viewItr;
            ++viewNameItr;
        }
    }

    { // pre texture allocation
        int textureCount = 0;
        for(View& view : part.views)
        {
            textureCount += (int)std::ssize(view.subViewLayouts);
        }

        textureAllocator.preAllocation(textureCount);
    }

    { // texture allocation
        int textureIndex = 0;
        for(View& view : part.views)
        {
            for(SubViewLayout& subViewLayout : view.subViewLayouts)
            {
                if(!createTextureForLayout(textureAllocator, textureIndex, subViewLayout, properties.dimensions.x,
                                           properties.dimensions.y, properties.mips))
                {
                    return 0;
                }

                ++textureIndex;
            }
        }

        textureAllocator.postAllocation();
    }

    { // tetxure fill
        const cputex::Extent textureExtent{properties.dimensions.x, properties.dimensions.y, 1};

        for(View& view : part.views)
        {
            for(SubViewLayout& subViewLayout : view.subViewLayouts)
            {
                for(int mip = 0; mip < properties.mips; ++mip)
                {
                    std::span<std::byte> mipData = textureAllocator.accessTextureData(
                        subViewLayout.textureIndex, MipSurfaceKey{.arraySlice = 0, .face = 0, .mip = (int8_t)mip});

                    frameBufferCount += fillFrameBuffer(frameBuffers[mip], properties.drawDataMin, subViewLayout,
                                                        cputex::calculateMipExtent(textureExtent, mip), mipData);
                }
            }
        }
    }

    return frameBufferCount;
}
} // namespace teximp::exr

#endif // TEXIMP_ENABLE_EXR_BACKEND_OPENEXR