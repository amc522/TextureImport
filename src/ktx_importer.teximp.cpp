#include <teximp/ktx/ktx_importer.teximp.h>

#ifdef TEXIMP_ENABLE_KTX_BACKEND_TEXIMP

#include <gl/glcorearb.h>
#include <gl/glext.h>
#include <glm/gtc/round.hpp>

#ifdef TEXIMP_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable :4063)
#endif

#include <gpufmt/opengl.h>

#ifdef TEXIMP_COMPILER_MSVC
#pragma warning(pop)
#endif

namespace teximp::ktx
{
std::span<const char> KtxTexImpImporter::fileIdentifier() const
{
    return mFileIdentifier;
}

const ktx::header10& KtxTexImpImporter::header() const
{
    return mHeader;
}

const std::vector<uint8_t>& KtxTexImpImporter::keyValueData() const
{
    return mKeyValueData;
}

const std::vector<KtxTexImpImporter::KeyValuePair>& KtxTexImpImporter::keyValuePairs() const
{
    return mKeyValuePairs;
}

FileFormat KtxTexImpImporter::fileFormat() const
{
    return FileFormat::Ktx;
}

bool KtxTexImpImporter::checkSignature(std::istream& stream)
{
    static constexpr std::array<char, 12> fileIdentifier = {'«', 'K', 'T',  'X',  ' ',    '1',
                                                            '1', '»', '\r', '\n', '\x1A', '\n'};

    stream.read(mFileIdentifier.data(), mFileIdentifier.size());

    if(stream.fail()) { return false; }

    if(memcmp(mFileIdentifier.data(), fileIdentifier.data(), fileIdentifier.size()) != 0) { return false; }

    return true;
}

void KtxTexImpImporter::load(std::istream& stream, ITextureAllocator& textureAllocator,
                             TextureImportOptions /*options*/)
{
    stream.read(reinterpret_cast<char*>(&mHeader), sizeof(ktx::header10));

    if(stream.fail())
    {
        mStatus = TextureImportStatus::Error;
        mError = TextureImportError::Unknown;
        mErrorMessage = "Not enough data for the ktx header";
        return;
    }

    mKeyValueData.resize(mHeader.BytesOfKeyValueData);
    stream.read(reinterpret_cast<char*>(mKeyValueData.data()), mHeader.BytesOfKeyValueData);

    uint32_t keyValueDataOffset = 0;

    if(mHeader.BytesOfKeyValueData > 0)
    {
        while(keyValueDataOffset <= mKeyValueData.size() - sizeof(uint32_t))
        {
            uint32_t keyValueBytes = (mKeyValueData.data() + keyValueDataOffset)[0];
            keyValueDataOffset += sizeof(uint32_t);

            std::span<uint8_t> keyValueSpan(mKeyValueData.data() + keyValueDataOffset, keyValueBytes);
            KeyValuePair keyValuePair;

            uint32_t keyEnd;
            for(keyEnd = 0; keyEnd < keyValueSpan.size_bytes(); ++keyEnd)
            {
                if(keyValueSpan[keyEnd] == '\0') { break; }
            }

            keyValuePair.first = std::string_view((const char*)keyValueSpan.data(), keyEnd + 1);
            keyValuePair.second = keyValueSpan.last(keyValueBytes - keyEnd - 1);
            mKeyValuePairs.emplace_back(keyValuePair);

            uint32_t padding = 3 - ((keyValueBytes + 3) % 4);
            keyValueDataOffset += keyValueBytes + padding;
        }
    }

    std::optional format = gpufmt::gl::translateFormat(mHeader.GLInternalFormat, mHeader.GLFormat, mHeader.GLType);

    if(!format)
    {
        mStatus = TextureImportStatus::Error;
        mError = TextureImportError::UnknownFormat;
        mErrorMessage = "Invalid OpenGL format";
        return;
    }

    const uint32_t BlockSize = gpufmt::formatInfo(format.value()).blockByteSize;

    cputex::TextureParams textureParams{
        .format = format.value(),
        .dimension = ktx::getTextureDimension(mHeader),
        .extent = {mHeader.PixelWidth, std::max<uint32_t>(mHeader.PixelHeight, 1u),
                   std::max<uint32_t>(mHeader.PixelDepth, 1u)},
        .arraySize = std::max((cputex::CountType)mHeader.NumberOfArrayElements, cputex::CountType(1)),
        .faces = std::max((cputex::CountType)mHeader.NumberOfFaces, cputex::CountType(1)),
        .mips = std::max((cputex::CountType)mHeader.NumberOfMipmapLevels, cputex::CountType(1))
    };

    textureAllocator.preAllocation(1);

    if(!textureAllocator.allocateTexture(textureParams, 0))
    {
        setTextureAllocationError(textureParams);
        return;
    }

    textureAllocator.postAllocation();

    for(uint32_t mip = 0, mips = textureParams.mips; mip < mips; ++mip)
    {
        uint32_t imageSize;
        stream.read(reinterpret_cast<char*>(&imageSize), sizeof(uint32_t));

        if(stream.fail())
        {
            mStatus = TextureImportStatus::Error;
            mError = TextureImportError::Unknown;
            mErrorMessage = "Expected larger file size";
            return;
        }

        for(uint32_t arraySlice = 0, arraySize = textureParams.arraySize; arraySlice < arraySize; ++arraySlice)
        {
            for(uint32_t face = 0, faces = textureParams.faces; face < faces; ++face)
            {
                std::span<std::byte> surfaceSpan = textureAllocator.accessTextureData(
                    0, MipSurfaceKey{.arraySlice = (int16_t)arraySlice, .face = (int8_t)face, .mip = (int8_t)mip});

                stream.read(reinterpret_cast<char*>(surfaceSpan.data()),
                            std::min(imageSize, static_cast<uint32_t>(surfaceSpan.size_bytes())));

                if(stream.fail())
                {
                    mStatus = TextureImportStatus::Error;
                    mError = TextureImportError::Unknown;
                    mErrorMessage = "Expected larger file size";
                    return;
                }

                size_t offset = std::max(static_cast<size_t>(BlockSize),
                                         glm::ceilMultiple(surfaceSpan.size_bytes(), static_cast<size_t>(4))) -
                                surfaceSpan.size_bytes();
                stream.seekg(offset, std::ios_base::cur);
            }
        }
    }
}
} // namespace teximp::ktx

#endif // TEXIMP_ENABLE_KTX_BACKEND_TEXIMP