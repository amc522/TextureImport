#include <teximp/tiff/tiff_importer.tiff.h>

#ifdef TEXIMP_ENABLE_TIFF_BACKEND_TIFF

#include <cputex/texture_operations.h>
#include <glm/glm.hpp>
#include <tiff.h>
#include <tiffio.h>
#include <tiffio.hxx>

#include <gsl/gsl-lite.hpp>

namespace teximp::tiff
{
FileFormat TiffTexImpImporter::fileFormat() const
{
    return FileFormat::Tiff;
}

bool TiffTexImpImporter::checkSignature(std::istream& stream)
{
    TiffHeader header;
    stream.read((char*)(&header), sizeof(header));

    if(header.identifier == 0x4949 && header.version == 42) { return true; }
    else if(header.identifier == 0x4d4d && header.version == 0x2a00) { return true; }

    return false;
}

gpufmt::Format getFormat(uint32_t channelCount, uint32_t bitsPerChannel, uint32_t dataType)
{
    bool dataTypeIsValid =
        dataType == SAMPLEFORMAT_INT || dataType == SAMPLEFORMAT_UINT || dataType == SAMPLEFORMAT_IEEEFP;
    bool bitsPerChannelIsValid = bitsPerChannel >= 8;
    bool channelCountIsValid = channelCount > 0 && channelCount <= 4;

    if(!dataTypeIsValid || !bitsPerChannelIsValid || !channelCountIsValid) { return gpufmt::Format::R8G8B8A8_UNORM; }

    if(channelCount == 1 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_UINT) { return gpufmt::Format::R8_UNORM; }
    else if(channelCount == 1 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_INT)
    {
        return gpufmt::Format::R8_SNORM;
    }
    else if(channelCount == 1 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        return gpufmt::Format::R8_UNORM;
    }
    else if(channelCount == 1 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_UINT)
    {
        return gpufmt::Format::R16_UNORM;
    }
    else if(channelCount == 1 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_INT)
    {
        return gpufmt::Format::R16_SNORM;
    }
    else if(channelCount == 1 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        return gpufmt::Format::R16_SFLOAT;
    }
    else if(channelCount == 2 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_UINT)
    {
        return gpufmt::Format::R8G8_UNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_INT)
    {
        return gpufmt::Format::R8G8_SNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        return gpufmt::Format::R8G8_UNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_UINT)
    {
        return gpufmt::Format::R16G16_UNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_INT)
    {
        return gpufmt::Format::R16G16_SNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        return gpufmt::Format::R16G16_SFLOAT;
    }
    else if(channelCount == 3 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_UINT)
    {
        return gpufmt::Format::R8G8B8_UNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_INT)
    {
        return gpufmt::Format::R8G8B8_SNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        return gpufmt::Format::R8G8B8_UNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_UINT)
    {
        return gpufmt::Format::R16G16B16_UNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_INT)
    {
        return gpufmt::Format::R16G16B16_SNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        return gpufmt::Format::R16G16B16_SFLOAT;
    }
    else if(channelCount == 4 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_UINT)
    {
        return gpufmt::Format::R8G8B8A8_UNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_INT)
    {
        return gpufmt::Format::R8G8B8A8_SNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        return gpufmt::Format::R8G8B8A8_UNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_UINT)
    {
        return gpufmt::Format::R16G16B16A16_UNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_INT)
    {
        return gpufmt::Format::R16G16B16A16_SNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        return gpufmt::Format::R16G16B16A16_SFLOAT;
    }
    else { return gpufmt::Format::R8G8B8A8_UNORM; }
}

void TiffTexImpImporter::load(std::istream& stream, ITextureAllocator& textureAllocator,
                              TextureImportOptions /*options*/)
{
    stream.seekg(0);

    struct ErrorData
    {
        bool error = false;
        std::string message;
    } errorData;

    TIFFSetErrorHandlerExt(
        [](thandle_t clientData, const char* /*module*/, const char* fmt, va_list args)
        {
#ifdef TEXIMP_COMPILER_MSVC
            std::string errorMessage(_vscprintf(fmt, args), '\0');
            vsprintf_s(errorMessage.data(), errorMessage.size() + 1, fmt, args);

            ((TiffTexImpImporter*)clientData)->setErrorMessage(std::move(errorMessage));
#endif
        });

    TIFF* tiffHandle = TIFFStreamOpen(mFilePath.string().c_str(), &stream);

    if(tiffHandle == nullptr)
    {
        setError(TextureImportError::FailedToOpenFile, "Could not open tiff file.");
        return;
    }

    TIFFSetClientdata(tiffHandle, this);

    auto scopeExit = gsl::finally([tiffHandle]() { TIFFClose(tiffHandle); });

    uint32_t width = 0;
    uint32_t height = 0;

    TIFFGetField(tiffHandle, TIFFTAG_IMAGEWIDTH, &width);
    TIFFGetField(tiffHandle, TIFFTAG_IMAGELENGTH, &height);

    uint32_t tileWidth = width;
    uint32_t tileHeight = height;

    TIFFGetField(tiffHandle, TIFFTAG_TILEWIDTH, &tileWidth);
    TIFFGetField(tiffHandle, TIFFTAG_TILELENGTH, &tileHeight);

    uint16_t planarConfig = 0;
    TIFFGetField(tiffHandle, TIFFTAG_PLANARCONFIG, &planarConfig);

    uint32_t bitsPerChannel = 0;
    TIFFGetField(tiffHandle, TIFFTAG_BITSPERSAMPLE, &bitsPerChannel);

    uint32_t channelCount = 0;
    TIFFGetField(tiffHandle, TIFFTAG_SAMPLESPERPIXEL, &channelCount);

    uint32_t pixelDataType = 0;
    TIFFGetField(tiffHandle, TIFFTAG_SAMPLEFORMAT, &pixelDataType);

    uint32_t compression = 0;
    TIFFGetField(tiffHandle, TIFFTAG_COMPRESSION, &compression);

    uint32_t photometric = 0;
    TIFFGetField(tiffHandle, TIFFTAG_PHOTOMETRIC, &photometric);

    // uint32_t numDirs = TIFFNumberOfDirectories(tiffHandle);
    uint32_t tileCount = glm::min(TIFFNumberOfTiles(tiffHandle),
                                  ((width + width - 1) / tileWidth) * ((height + height - 1) / tileHeight));

    // uint32_t stipSize = TIFFStripSize(tiffHandle);
    tmsize_t scanlineSize = TIFFScanlineSize(tiffHandle);
    gpufmt::Format format;

    if(compression == COMPRESSION_NONE && photometric == PHOTOMETRIC_RGB)
    {
        format = getFormat(channelCount, bitsPerChannel, pixelDataType);
    }
    else { format = gpufmt::Format::R8G8B8A8_UNORM; }

    cputex::TextureParams params;
    params.dimension = cputex::TextureDimension::Texture2D;
    params.format = format;
    params.extent = {width, height, 1};
    params.arraySize = 1u;
    params.faces = 1u;
    params.mips = 1u;

    textureAllocator.preAllocation(1);

    if(!textureAllocator.allocateTexture(params, 0))
    {
        setTextureAllocationError(params);
        return;
    }

    textureAllocator.postAllocation();

    std::span<std::byte> surfaceSpan = textureAllocator.accessTextureData(0, {});

    if(tileCount == 1)
    {
        if(format == gpufmt::Format::R8G8B8A8_UNORM)
        {
            int ret = TIFFReadRGBAImageOriented(tiffHandle, width, height,
                                                castWritableBytes<uint32_t>(surfaceSpan).data(), ORIENTATION_LEFTTOP);
            if(ret < 1)
            {
                setError(TextureImportError::Unknown, "Failed to read image");
                return;
            }
        }
        else
        {
            for(int row = height - 1; row >= 0; --row)
            {
                TIFFReadScanline(tiffHandle, surfaceSpan.subspan(row * scanlineSize).data(), row);
            }
        }
    }
    else
    {
        cputex::TextureParams tileParams;
        tileParams.dimension = cputex::TextureDimension::Texture2D;
        tileParams.extent = {tileWidth, tileHeight, 1};
        tileParams.arraySize = 1u;
        tileParams.faces = 1u;
        tileParams.mips = 1u;
        tileParams.format = format;

        cputex::UniqueTexture tileTexture(tileParams);

        size_t tileIndex = 0;
        for(uint32_t y = 0; y < height; y += tileHeight)
        {
            for(uint32_t x = 0; x < width; x += tileWidth)
            {
                tmsize_t bytesDecoded;
                if(format == gpufmt::Format::R8G8B8A8_UNORM)
                {
                    bytesDecoded =
                        TIFFReadRGBATile(tiffHandle, x, y, tileTexture.accessMipSurfaceDataAs<uint32_t>().data());
                }
                else
                {
                    bytesDecoded =
                        TIFFReadTile(tiffHandle, tileTexture.accessMipSurfaceDataAs<uint32_t>().data(), x, y, 0, 0);
                }

                if(bytesDecoded <= 0)
                {
                    setError(TextureImportError::Unknown, "Failed to read tiled image.");
                    return;
                }

                cputex::Extent destSize{glm::min(tileWidth, width - x), glm::min(tileHeight, height - y), 1u};
                glm::uvec3 destOffset(x, y, 0);

                cputex::flipHorizontal(static_cast<cputex::TextureSpan>(tileTexture));
                cputex::copySurfaceRegionTo(
                    (cputex::SurfaceView)tileTexture.getMipSurface(0, 0, 0), glm::uvec3{0, 0, 0},
                    (cputex::SurfaceSpan)tileTexture.accessMipSurface(0, 0, 0), destOffset, destSize);

                ++tileIndex;
            }
        }
    }
}
} // namespace teximp::tiff

#endif // TEXIMP_ENABLE_TIFF_BACKEND_TIFF