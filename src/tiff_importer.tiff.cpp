#include <teximp/tiff/tiff_importer.tiff.h>

#ifdef TEXIMP_ENABLE_TIFF_BACKEND_TIFF

#include <cputex/texture_operations.h>
#include <glm/glm.hpp>
#include <gpufmt/utility.h>
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

gpufmt::Format getFormat(uint32_t channelCount, uint32_t bitsPerChannel, uint32_t dataType,
                         TextureImportOptions options)
{
    if(dataType == SAMPLEFORMAT_VOID && bitsPerChannel == 8) { dataType = SAMPLEFORMAT_UINT; }

    bool dataTypeIsValid =
        dataType == SAMPLEFORMAT_INT || dataType == SAMPLEFORMAT_UINT || dataType == SAMPLEFORMAT_IEEEFP;
    bool bitsPerChannelIsValid = bitsPerChannel >= 8;
    bool channelCountIsValid = channelCount > 0 && channelCount <= 4;

    if(!dataTypeIsValid || !bitsPerChannelIsValid || !channelCountIsValid) { return gpufmt::Format::UNDEFINED; }

    gpufmt::Format format = gpufmt::Format::UNDEFINED;

    if(channelCount == 1 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_UINT) { format = gpufmt::Format::R8_UNORM; }
    else if(channelCount == 1 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_INT)
    {
        format = gpufmt::Format::R8_SNORM;
    }
    else if(channelCount == 1 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        format = gpufmt::Format::R8_UNORM;
    }
    else if(channelCount == 1 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_UINT)
    {
        format = gpufmt::Format::R16_UNORM;
    }
    else if(channelCount == 1 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_INT)
    {
        format = gpufmt::Format::R16_SNORM;
    }
    else if(channelCount == 1 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        format = gpufmt::Format::R16_SFLOAT;
    }
    else if(channelCount == 2 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_UINT)
    {
        format = gpufmt::Format::R8G8_UNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_INT)
    {
        format = gpufmt::Format::R8G8_SNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        format = gpufmt::Format::R8G8_UNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_UINT)
    {
        format = gpufmt::Format::R16G16_UNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_INT)
    {
        format = gpufmt::Format::R16G16_SNORM;
    }
    else if(channelCount == 2 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        format = gpufmt::Format::R16G16_SFLOAT;
    }
    else if(channelCount == 3 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_UINT)
    {
        format = (options.padRgbWithAlpha) ? gpufmt::Format::R8G8B8A8_UNORM : gpufmt::Format::R8G8B8_UNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_INT)
    {
        format = (options.padRgbWithAlpha) ? gpufmt::Format::R8G8B8A8_SNORM : gpufmt::Format::R8G8B8_SNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        format = (options.padRgbWithAlpha) ? gpufmt::Format::R8G8B8A8_UNORM : gpufmt::Format::R8G8B8_UNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_UINT)
    {
        format = gpufmt::Format::R16G16B16_UNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_INT)
    {
        format = gpufmt::Format::R16G16B16_SNORM;
    }
    else if(channelCount == 3 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        format = gpufmt::Format::R16G16B16_SFLOAT;
    }
    else if(channelCount == 4 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_UINT)
    {
        format = gpufmt::Format::R8G8B8A8_UNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_INT)
    {
        format = gpufmt::Format::R8G8B8A8_SNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 8 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        format = gpufmt::Format::R8G8B8A8_UNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_UINT)
    {
        format = gpufmt::Format::R16G16B16A16_UNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_INT)
    {
        format = gpufmt::Format::R16G16B16A16_SNORM;
    }
    else if(channelCount == 4 && bitsPerChannel == 16 && dataType == SAMPLEFORMAT_IEEEFP)
    {
        format = gpufmt::Format::R16G16B16A16_SFLOAT;
    }

    if(options.assumeSrgb) { format = gpufmt::sRGBFormat(format).value_or(format); }

    return format;
}

// BEGIN TIFFIO SECTION
//
// TIFFIO was copied here because a pointer to TiffTiffImporter was needed in the client
// data for setting error messages. So the code has been copied here and slightly modified.

// clang-format off
/*
 * Copyright (c) 1988-1996 Sam Leffler
 * Copyright (c) 1991-1996 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */
// clang-format on

struct TiffClientData
{
    std::istream* stream = nullptr;
    std::ios::pos_type startPos{};
    TiffTexImpImporter* importer = nullptr;
};

tmsize_t tiffRead(thandle_t fd, void* buf, tmsize_t size)
{
    TiffClientData* data = reinterpret_cast<TiffClientData*>(fd);

    // Verify that type does not overflow.
    std::streamsize requestSize = size;
    if(static_cast<tmsize_t>(requestSize) != size) { return static_cast<tmsize_t>(-1); }

    data->stream->read((char*)buf, requestSize);

    return static_cast<tmsize_t>(data->stream->gcount());
}

tmsize_t tiffDummyWrite(thandle_t, void*, tmsize_t)
{
    return 0;
}

static uint64_t tiffSeek(thandle_t fd, uint64_t off, int whence)
{
    TiffClientData* data = reinterpret_cast<TiffClientData*>(fd);

    switch(whence)
    {
    case SEEK_SET:
    {
        // Compute 64-bit offset
        uint64_t new_offset = static_cast<uint64_t>(data->startPos) + off;

        // Verify that value does not overflow
        std::ios::off_type offset = static_cast<std::ios::off_type>(new_offset);
        if(static_cast<uint64_t>(offset) != new_offset) return static_cast<uint64_t>(-1);

        data->stream->seekg(offset, std::ios::beg);
        break;
    }
    case SEEK_CUR:
    {
        // Verify that value does not overflow
        std::ios::off_type offset = static_cast<std::ios::off_type>(off);
        if(static_cast<uint64_t>(offset) != off) return static_cast<uint64_t>(-1);

        data->stream->seekg(offset, std::ios::cur);
        break;
    }
    case SEEK_END:
    {
        // Verify that value does not overflow
        std::ios::off_type offset = static_cast<std::ios::off_type>(off);
        if(static_cast<uint64_t>(offset) != off) return static_cast<uint64_t>(-1);

        data->stream->seekg(offset, std::ios::end);
        break;
    }
    }

    return (uint64_t)(data->stream->tellg() - data->startPos);
}

static uint64_t tiffSize(thandle_t fd)
{
    TiffClientData* data = reinterpret_cast<TiffClientData*>(fd);
    std::ios::pos_type pos = data->stream->tellg();
    std::ios::pos_type len;

    data->stream->seekg(0, std::ios::end);
    len = data->stream->tellg();
    data->stream->seekg(pos);

    return (uint64_t)len;
}

static int tiffDummyClose(thandle_t /*fd*/)
{
    return 0;
}

static int tiffDummyMap(thandle_t, void** base, toff_t* size)
{
    (void)base;
    (void)size;
    return (0);
}

static void tiffDummyUnmap(thandle_t, void* base, toff_t size)
{
    (void)base;
    (void)size;
}

TIFF* tiffStreamOpen(std::filesystem::path& filePath, TiffClientData& clientData)
{
    clientData.startPos = clientData.stream->tellg();

    // Open for reading.
    TIFF* tif = TIFFClientOpen(filePath.string().c_str(), "rm", reinterpret_cast<thandle_t>(&clientData), tiffRead,
                               tiffDummyWrite, tiffSeek, tiffDummyClose, tiffSize, tiffDummyMap, tiffDummyUnmap);

    return tif;
}
// END TIFFIO SECTION

TextureParams createTextureParams(TIFF* tiffHandle, TextureImportOptions options, bool& isCmyk)
{
    uint32_t width = 0;
    uint32_t height = 0;

    int result;

    result = TIFFGetField(tiffHandle, TIFFTAG_IMAGEWIDTH, &width);
    result = TIFFGetField(tiffHandle, TIFFTAG_IMAGELENGTH, &height);

    uint32_t bitsPerChannel = 0;
    result = TIFFGetField(tiffHandle, TIFFTAG_BITSPERSAMPLE, &bitsPerChannel);

    uint32_t channelCount = 0;
    result = TIFFGetField(tiffHandle, TIFFTAG_SAMPLESPERPIXEL, &channelCount);

    uint32_t pixelDataType = 0;
    result = TIFFGetField(tiffHandle, TIFFTAG_SAMPLEFORMAT, &pixelDataType);
    if(result <= 0) { pixelDataType = SAMPLEFORMAT_UINT; }

    uint32_t photometric = 0;
    result = TIFFGetField(tiffHandle, TIFFTAG_PHOTOMETRIC, &photometric);

    uint32_t extraSamples;
    result = TIFFGetField(tiffHandle, TIFFTAG_EXTRASAMPLES, &extraSamples);

    gpufmt::Format format = gpufmt::Format::UNDEFINED;

    if(photometric != PHOTOMETRIC_SEPARATED && (channelCount == 3 || channelCount == 4))
    {
        format = getFormat(channelCount, bitsPerChannel, pixelDataType, options);
    }
    else if(photometric == PHOTOMETRIC_SEPARATED && channelCount == 4)
    {
        isCmyk = true;

        // probably cmyk
        switch(bitsPerChannel)
        {
        case 8: format = gpufmt::Format::R8G8B8A8_UNORM; break;
        case 16: format = gpufmt::Format::R16G16B16A16_UNORM; break;
        default: break;
        }
    }
    else { format = gpufmt::Format::R8G8B8A8_UNORM; }

    cputex::TextureParams params;
    params.dimension = cputex::TextureDimension::Texture2D;
    params.format = format;
    params.extent = {width, height, 1};
    params.arraySize = 1u;
    params.faces = 1u;
    params.mips = 1u;

    return params;
}

void TiffTexImpImporter::load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options)
{
    stream.seekg(0);

    TIFFSetErrorHandlerExt(
        [](thandle_t clientData, const char* /*module*/, const char* fmt, va_list args)
        {
#ifdef TEXIMP_COMPILER_MSVC
            constexpr std::string_view messagePrefix = "libtiff error: ";
            const auto errorMessageSize = _vscprintf(fmt, args);
            std::string errorMessage(errorMessageSize + messagePrefix.size(), '\0');
            errorMessage.assign(messagePrefix);
            vsprintf_s(errorMessage.data() + messagePrefix.size(), errorMessageSize + 1, fmt, args);

            ((TiffClientData*)clientData)->importer->setErrorMessage(std::move(errorMessage));
#endif
        });

    TIFFSetWarningHandler([](const char*, const char*, va_list) {});

    TiffClientData tiffClientData;
    tiffClientData.stream = &stream;
    tiffClientData.importer = this;

    TIFF* tiffHandle = tiffStreamOpen(mFilePath, tiffClientData);

    if(tiffHandle == nullptr)
    {
        setError(TextureImportError::FailedToOpenFile, "Could not open tiff file.");
        return;
    }

    auto scopeExit = gsl::finally([tiffHandle]() { TIFFClose(tiffHandle); });

    int numDirs = (int)TIFFNumberOfDirectories(tiffHandle);

    textureAllocator.preAllocation((int)numDirs);

    for(int i = 0; i < numDirs; ++i)
    {
        bool isCmyk;
        cputex::TextureParams params = createTextureParams(tiffHandle, options, isCmyk);

        if(params.format == gpufmt::Format::UNDEFINED)
        {
            setError(TextureImportError::UnknownFormat);
            return;
        }

        if(!textureAllocator.allocateTexture(params, i))
        {
            setTextureAllocationError(params);
            return;
        }

        TIFFReadDirectory(tiffHandle);
    }

    textureAllocator.postAllocation();

    TIFFSetDirectory(tiffHandle, 0);

    for(int i = 0; i < numDirs; ++i)
    {
        bool isCmyk;
        TextureParams params = createTextureParams(tiffHandle, options, isCmyk);

        std::span<std::byte> surfaceSpan = textureAllocator.accessTextureData(i, {});

        uint32_t tileCount = TIFFNumberOfTiles(tiffHandle);

        uint32_t tileWidth;
        uint32_t tileHeight;

        TIFFGetField(tiffHandle, TIFFTAG_TILEWIDTH, &tileWidth);
        TIFFGetField(tiffHandle, TIFFTAG_TILELENGTH, &tileHeight);

        tileCount = glm::min(TIFFNumberOfTiles(tiffHandle),
                             ((2 * params.extent.x - 1) / tileWidth) * ((2 * params.extent.y - 1) / tileHeight));

        if(tileCount <= 1)
        {
            if(params.format == gpufmt::Format::R8G8B8A8_UNORM || params.format == gpufmt::Format::R8G8B8A8_SRGB)
            {
                int ret =
                    TIFFReadRGBAImageOriented(tiffHandle, params.extent.x, params.extent.y,
                                              castWritableBytes<uint32_t>(surfaceSpan).data(), ORIENTATION_LEFTTOP);
                if(ret < 1)
                {
                    setError(TextureImportError::Unknown);
                    return;
                }
            }
            else
            {
                tmsize_t scanlineSize = TIFFScanlineSize(tiffHandle);

                for(int row = params.extent.y - 1; row >= 0; --row)
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
            tileParams.format = params.format;

            cputex::UniqueTexture tileTexture(tileParams);

            cputex::SurfaceSpan destSurface(params.format, cputex::TextureDimension::Texture2D, params.extent,
                                            surfaceSpan);

            size_t tileIndex = 0;
            for(cputex::ExtentComponent y = 0; y < params.extent.y; y += tileHeight)
            {
                for(cputex::ExtentComponent x = 0; x < params.extent.x; x += tileWidth)
                {
                    tmsize_t bytesDecoded;
                    if(params.format == gpufmt::Format::R8G8B8A8_UNORM ||
                       params.format == gpufmt::Format::R8G8B8A8_SRGB)
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

                    cputex::Extent destSize{glm::min((cputex::ExtentComponent)tileWidth, params.extent.x - x),
                                            glm::min((cputex::ExtentComponent)tileHeight, params.extent.y - y),
                                            cputex::ExtentComponent(1)};
                    cputex::Extent destOffset(x, y, 0);

                    cputex::flipHorizontal(static_cast<cputex::TextureSpan>(tileTexture));
                    cputex::copySurfaceRegionTo((cputex::SurfaceView)tileTexture.getMipSurface(0, 0, 0),
                                                cputex::Extent{0, 0, 0}, destSurface, destOffset, destSize);

                    ++tileIndex;
                }
            }
        }

        if(isCmyk && params.format == gpufmt::Format::R16G16B16A16_UNORM)
        {
            cputex::SurfaceSpan surface(params.format, cputex::TextureDimension::Texture2D, params.extent, surfaceSpan);
            cputex::transform(surface,
                              [](gpufmt::Format /*format*/, std::span<std::byte> block)
                              {
                                  const auto cmyk16 = castWritableBytes<glm::u16vec4>(block).front();
                                  auto& rgba16 = castWritableBytes<glm::u16vec4>(block).front();

                                  constexpr uint16_t unorm16Max = std::numeric_limits<uint16_t>::max();
                                  constexpr float unorm16Maxf = static_cast<float>(unorm16Max);
                                  constexpr float invUnorm16Max = 1.0f / unorm16Maxf;

                                  const glm::vec4 cmyk(cmyk16.x * invUnorm16Max, cmyk16.y * invUnorm16Max,
                                                       cmyk16.z * invUnorm16Max, cmyk16.w * invUnorm16Max);

                                  rgba16.r = (uint16_t)std::round(unorm16Maxf * ((1.0f - cmyk.x) * (1.0f - cmyk.w)));
                                  rgba16.g = (uint16_t)std::round(unorm16Maxf * ((1.0f - cmyk.y) * (1.0f - cmyk.w)));
                                  rgba16.b = (uint16_t)std::round(unorm16Maxf * ((1.0f - cmyk.z) * (1.0f - cmyk.w)));
                                  rgba16.a = unorm16Max;
                              });
        }

        TIFFReadDirectory(tiffHandle);
    }
}
} // namespace teximp::tiff

#endif // TEXIMP_ENABLE_TIFF_BACKEND_TIFF