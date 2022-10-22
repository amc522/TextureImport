#include <teximp/png/png_importer.libpng.h>

#ifdef TEXIMP_ENABLE_PNG_BACKEND_LIBPNG

#include <png.h>

#include <bit>

#include <gsl/gsl-lite.hpp>

namespace teximp::png
{
FileFormat PngLibPngImporter::fileFormat() const
{
    return FileFormat::Png;
}

bool PngLibPngImporter::checkSignature(std::istream& stream)
{
    if(!stream)
    {
        mStatus = TextureImportStatus::Error;
        mError = TextureImportError::FailedToOpenFile;
        return false;
    }

    png_byte pngSignature[8];

    stream.read((char*)pngSignature, 8);
    return png_sig_cmp(pngSignature, 0, 8) == 0;
}

void PngLibPngImporter::load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options)
{
    try
    {
        png_structp pngRead = nullptr;
        png_infop pngInfo = nullptr;
        png_infop pngEndInfo = nullptr;

        auto scopeCleanup = gsl::finally([&pngRead, &pngInfo, &pngEndInfo]()
                                         { png_destroy_read_struct(&pngRead, &pngInfo, &pngEndInfo); });

        pngRead = png_create_read_struct(
            PNG_LIBPNG_VER_STRING, nullptr,
            [](png_structp /*pngPtr*/, png_const_charp message) { throw std::exception(message); }, nullptr);

        if(pngRead == nullptr)
        {
            mStatus = TextureImportStatus::Error;
            mError = TextureImportError::Unknown;
            return;
        }

        pngInfo = png_create_info_struct(pngRead);

        if(pngInfo == nullptr)
        {
            mStatus = TextureImportStatus::Error;
            mError = TextureImportError::Unknown;
            return;
        }

        png_set_read_fn(pngRead, &stream,
                        [](png_structp pngRead, png_bytep data, png_size_t length)
                        {
                            std::istream& stream = *static_cast<std::istream*>(png_get_io_ptr(pngRead));
                            stream.read((char*)data, length);
                        });

        png_set_sig_bytes(pngRead, 8);

        png_set_user_limits(pngRead, 16384, 16384);
        png_read_info(pngRead, pngInfo);

        uint32_t width;
        uint32_t height;
        int bitDepth;
        int colorType;
        int interlaceType;
        int compressionType;
        int filterMethod;
        uint8_t channelCount;
        bool hasAlpha = false;

        png_get_IHDR(pngRead, pngInfo, &width, &height, &bitDepth, &colorType, &interlaceType, &compressionType,
                     &filterMethod);
        channelCount = png_get_channels(pngRead, pngInfo);

        hasAlpha = channelCount == 4 || colorType == PNG_COLOR_TYPE_GRAY_ALPHA || colorType == PNG_COLOR_TYPE_RGBA;

        if(bitDepth == 16 && std::endian::native == std::endian::little) { png_set_swap(pngRead); }

        if(!hasAlpha && options.padRgbWithAlpha)
        {
            png_set_add_alpha(pngRead, 0xFFFFFFFF, PNG_FILLER_AFTER);
            hasAlpha = true;
        }

        gpufmt::Format gpuFormat = gpufmt::Format::UNDEFINED;

        switch(colorType)
        {
        case PNG_COLOR_TYPE_PALETTE:
            png_set_palette_to_rgb(pngRead);
            channelCount = 3;

            if(hasAlpha) { gpuFormat = gpufmt::Format::R8G8B8A8_SRGB; }
            else { gpuFormat = gpufmt::Format::R8G8B8_SRGB; }
            break;
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            png_set_gray_to_rgb(pngRead);
            if(bitDepth < 8)
            {
                png_set_expand_gray_1_2_4_to_8(pngRead);
                gpuFormat = (hasAlpha) ? gpufmt::Format::R8G8B8A8_SRGB : gpufmt::Format::R8G8B8_SRGB;
            }
            else if(bitDepth == 8)
            {
                gpuFormat = (hasAlpha) ? gpufmt::Format::R8G8B8A8_SRGB : gpufmt::Format::R8G8B8_SRGB;
            }
            else if(bitDepth == 16)
            {
                gpuFormat = (hasAlpha) ? gpufmt::Format::R16G16B16A16_UNORM : gpufmt::Format::R16G16B16_UNORM;
            }

            break;
        case PNG_COLOR_TYPE_RGB:
            if(!hasAlpha)
            {
                if(bitDepth == 8) { gpuFormat = gpufmt::Format::R8G8B8_SRGB; }
                else if(bitDepth == 16) { gpuFormat = gpufmt::Format::R16G16B16_UNORM; }
            }
            else
            {
                if(bitDepth == 8) { gpuFormat = gpufmt::Format::R8G8B8A8_SRGB; }
                else if(bitDepth == 16) { gpuFormat = gpufmt::Format::R16G16B16A16_UNORM; }
            }
            break;
        case PNG_COLOR_TYPE_RGBA:
            if(bitDepth == 8) { gpuFormat = gpufmt::Format::R8G8B8A8_SRGB; }
            else if(bitDepth == 16) { gpuFormat = gpufmt::Format::R16G16B16A16_UNORM; }
            break;
        }

        if(gpuFormat == gpufmt::Format::UNDEFINED)
        {
            mStatus = TextureImportStatus::Error;
            mError = TextureImportError::Unknown;
            return;
        }

        png_read_update_info(pngRead, pngInfo);

        cputex::TextureParams textureParams{
            .format = gpuFormat,
            .dimension = cputex::TextureDimension::Texture2D,
            .extent = {width, height, 1},
            .arraySize = 1,
            .faces = 1,
            .mips = 1
        };

        textureAllocator.preAllocation(1);

        if(!textureAllocator.allocateTexture(textureParams, 0))
        {
            setTextureAllocationError(textureParams);
            return;
        }

        std::span<std::byte> surfaceSpan = textureAllocator.accessTextureData(0, {});

        std::vector<png_byte*> rows(height);
        size_t blockSize = gpufmt::formatInfo(gpuFormat).blockByteSize;
        size_t offset = 0;
        for(size_t row = 0; row < height; ++row)
        {
            rows[row] = castWritableBytes<png_byte>(surfaceSpan.subspan(offset)).data();
            offset += width * blockSize;
        }

        png_read_image(pngRead, rows.data());
    }
    catch(const std::exception& exception)
    {
        mStatus = TextureImportStatus::Error;
        mError = TextureImportError::Unknown;
        mErrorMessage = exception.what();
    }
}
} // namespace teximp::png

#endif // TEXIMP_ENABLE_PNG_BACKEND_LIBPNG