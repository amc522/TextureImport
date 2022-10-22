#pragma once

#include <teximp/teximp.h>

#ifdef TEXIMP_ENABLE_EXR_BACKEND_OPENEXR

#ifdef TEXIMP_COMPILER_MSVC
#pragma warning(push)
#pragma warning( \
    disable :4996) // C4996	'strncpy': This function or variable may be unsafe.Consider using strncpy_s instead.To
                   // disable deprecation, use _CRT_SECURE_NO_WARNINGS.See online help for details.
#endif

#include <Imath/ImathBox.h>
#include <Imath/ImathMath.h>
#include <Imath/ImathMatrix.h>
#include <Imath/ImathNamespace.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfChromaticities.h>
#include <OpenEXR/ImfCompression.h>
#include <OpenEXR/ImfDeepImageState.h>
#include <OpenEXR/ImfEnvmap.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfKeyCode.h>
#include <OpenEXR/ImfLineOrder.h>
#include <OpenEXR/ImfRational.h>
#include <OpenEXR/ImfTileDescription.h>
#include <OpenEXR/ImfTimeCode.h>

#ifdef TEXIMP_COMPILER_MSVC
#pragma warning(pop)
#endif

#include <set>
#include <variant>
#include <vector>

OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_ENTER
class FrameBuffer;
class StdIStream;
OPENEXR_IMF_INTERNAL_NAMESPACE_HEADER_EXIT

namespace teximp::exr
{
class ExrOpenExrImporter : public TextureImporter
{
public:
    struct ChannelList
    {
        int channelCount;
    };

    struct PreviewImage
    {
        unsigned int width;
        unsigned int height;
    };

    struct UnknownAttributeType
    {
        std::string typeName;
        std::string bytes;
    };

    using AttributeVariant =
        std::variant<float, std::vector<float>, int, std::string, std::vector<std::string>,

                     // enums
                     Imf::Compression, Imf::DeepImageState, Imf::Envmap, Imf::LineOrder,

                     // structs
                     ChannelList, Imf::Chromaticities, Imf::KeyCode, PreviewImage, Imf::Rational, Imf::TileDescription,
                     Imf::TimeCode,

                     // math types
                     Imath::Box2f, Imath::Box2i, Imath::M33d, Imath::M33f, Imath::M44d, Imath::M44f, Imath::V2d,
                     Imath::V2f, Imath::V2i, Imath::V3d, Imath::V3f, Imath::V3i, Imath::V4d, Imath::V4f, Imath::V4i,

                     UnknownAttributeType>;

    struct ChannelPair
    {
        std::string name;
        Imf::Channel channel;
    };

    struct SubViewLayout
    {
        int textureIndex = -1;
        std::array<ChannelPair, 4> channels;
        gpufmt::ChannelMask channelMask = gpufmt::ChannelMask::None;
        Imf::PixelType pixelType;
    };

    struct View
    {
        std::string name;
        std::vector<SubViewLayout> subViewLayouts;
    };

    struct Part
    {
        Imf::Header header;
        std::set<std::string> layerNames;
        std::vector<std::string> viewNames;
        std::vector<View> views;
        std::map<std::string, AttributeVariant> attributes;
        int totalSubViews = 0;
    };

    FileFormat fileFormat() const final;

    const std::span<const Part> parts() const { return mParts; }

protected:
    bool checkSignature(std::istream& stream) final;
    void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) final;

    void loadDeepImage(Imf::StdIStream& exrStream, ITextureAllocator& textureAllocator, TextureImportOptions options);
    void loadImage(Imf::StdIStream& exrStream, ITextureAllocator& textureAllocator, TextureImportOptions options);
    void loadMultiPartImage(Imf::StdIStream& exrStream, ITextureAllocator& textureAllocator,
                            TextureImportOptions options);
    void loadTiledImage(Imf::StdIStream& exrStream, ITextureAllocator& textureAllocator, TextureImportOptions options);

private:
    struct TileProperties
    {
        glm::ivec2 coordinates;
        int mips = 0;
        int frameBufferOffset = 0;
    };

    struct Properties
    {
        glm::ivec2 dimensions{0, 0};
        glm::ivec2 drawDataMin{0, 0};
        int mips = 1;
    };

    bool createTextureForLayout(ITextureAllocator& textureAllocator, int textureIndex,
                                ExrOpenExrImporter::SubViewLayout& layout, int width, int height, int mips);

    int extractAllLayouts(const Properties& properties, Part& part, std::span<Imf::FrameBuffer> frameBuffers,
                          ITextureAllocator& textureAllocator);

    std::vector<Part> mParts;
    std::set<size_t> mResolvedChannels;
};
} // namespace teximp::exr

#endif // TEXIMP_ENABLE_EXR_BACKEND_OPENEXR