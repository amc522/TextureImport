#include <teximp/png/png_importer.libpng.h>

#ifdef TEXIMP_ENABLE_PNG_BACKEND_LIBPNG

#include <gpufmt/string.h>
#include <png.h>
#include <teximp/string.h>

#include <bit>

#include <gsl/gsl-lite.hpp>

namespace teximp::png
{
constexpr std::span<const gpufmt::Format> getFormatsForLayout(FormatLayout formatLayout, bool needsAlpha, bool sRGB)
{
    if(formatLayout == FormatLayout::_8_8_8)
    {
        return {
            {(sRGB) ? gpufmt::Format::R8G8B8_SRGB : gpufmt::Format::R8G8B8_UNORM,
             (sRGB) ? gpufmt::Format::B8G8R8_SRGB : gpufmt::Format::B8G8R8_UNORM}
        };
    }
    else if(formatLayout == FormatLayout::_8_8_8_8 && needsAlpha)
    {
        return {
            {
             (sRGB) ? gpufmt::Format::R8G8B8A8_SRGB : gpufmt::Format::R8G8B8A8_UNORM,
             (sRGB) ? gpufmt::Format::B8G8R8A8_SRGB : gpufmt::Format::B8G8R8A8_UNORM,
             (sRGB) ? gpufmt::Format::A8B8G8R8_SRGB_PACK32 : gpufmt::Format::A8B8G8R8_UNORM_PACK32,
             }
        };
    }
    else if(formatLayout == FormatLayout::_8_8_8_8 && !needsAlpha)
    {
        return {
            {
             (sRGB) ? gpufmt::Format::R8G8B8A8_SRGB : gpufmt::Format::R8G8B8A8_UNORM,
             (sRGB) ? gpufmt::Format::B8G8R8A8_SRGB : gpufmt::Format::B8G8R8A8_UNORM,
             (sRGB) ? gpufmt::Format::B8G8R8X8_SRGB : gpufmt::Format::B8G8R8X8_UNORM,
             (sRGB) ? gpufmt::Format::A8B8G8R8_SRGB_PACK32 : gpufmt::Format::A8B8G8R8_UNORM_PACK32,
             }
        };
    }
    else if(formatLayout == FormatLayout::_16_16_16) { return {{gpufmt::Format::R16G16B16_UNORM}}; }
    else if(formatLayout == FormatLayout::_16_16_16_16) { return {{gpufmt::Format::R16G16B16A16_UNORM}}; }

    return {};
}

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
        bool pngHasAlpha = false;

        png_get_IHDR(pngRead, pngInfo, &width, &height, &bitDepth, &colorType, &interlaceType, &compressionType,
                     &filterMethod);
        channelCount = png_get_channels(pngRead, pngInfo);

        pngHasAlpha = channelCount == 4 || colorType == PNG_COLOR_TYPE_GRAY_ALPHA || colorType == PNG_COLOR_TYPE_RGBA;

        int srgbIntent;
        const bool sRgb = png_get_sRGB(pngRead, pngInfo, &srgbIntent) == 0;

        if(bitDepth == 16 && std::endian::native == std::endian::little) { png_set_swap(pngRead); }

        if(!pngHasAlpha && options.padRgbWithAlpha) { png_set_add_alpha(pngRead, 0xFFFFFFFF, PNG_FILLER_AFTER); }

        const bool alphaNeeded = pngHasAlpha || options.padRgbWithAlpha;

        FormatLayout formatLayout;

        if(alphaNeeded && bitDepth <= 8)
        {
            constexpr std::array additionalLayouts = {FormatLayout::_16_16_16_16};

            formatLayout = textureAllocator.selectFormatLayout(FormatLayout::_8_8_8_8, additionalLayouts);

            if(formatLayout != FormatLayout::_8_8_8_8 && formatLayout != FormatLayout::_16_16_16_16)
            {
                setTextureAllocatorFormatLayoutError(formatLayout);
                return;
            }
        }
        else if(!alphaNeeded && bitDepth <= 8)
        {
            constexpr std::array additionalLayouts = {FormatLayout::_8_8_8_8, FormatLayout::_16_16_16,
                                                      FormatLayout::_16_16_16_16};

            formatLayout = textureAllocator.selectFormatLayout(FormatLayout::_8_8_8, additionalLayouts);

            if(formatLayout != FormatLayout::_8_8_8 && std::find(additionalLayouts.cbegin(), additionalLayouts.cend(),
                                                                 formatLayout) == additionalLayouts.cend())
            {
                setTextureAllocatorFormatLayoutError(formatLayout);
                return;
            }
        }
        else if(alphaNeeded && bitDepth == 16) { formatLayout = FormatLayout::_16_16_16_16; }
        else if(!alphaNeeded && bitDepth == 16)
        {
            constexpr std::array additionalLayouts = {FormatLayout::_16_16_16_16};

            formatLayout = textureAllocator.selectFormatLayout(FormatLayout::_16_16_16, additionalLayouts);

            if(formatLayout != FormatLayout::_16_16_16 && formatLayout != FormatLayout::_16_16_16_16)
            {
                setTextureAllocatorFormatLayoutError(formatLayout);
                return;
            }
        }
        else
        {
            setError(TextureImportError::UnknownFormat);
            return;
        }

        const std::span availableFormats = getFormatsForLayout(formatLayout, alphaNeeded, sRgb || options.assumeSrgb);
        const gpufmt::Format gpuFormat = textureAllocator.selectFormat(formatLayout, availableFormats);

        if(gpuFormat == gpufmt::Format::UNDEFINED)
        {
            setError(TextureImportError::UnknownFormat);
            return;
        }

        if(std::find(availableFormats.begin(), availableFormats.end(), gpuFormat) == availableFormats.end())
        {
            setTextureAllocatorFormatError(gpuFormat);
            return;
        }

        switch(colorType)
        {
        case PNG_COLOR_TYPE_PALETTE:
            png_set_palette_to_rgb(pngRead);
            channelCount = 3;
            break;
        case PNG_COLOR_TYPE_GRAY:
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            png_set_gray_to_rgb(pngRead);
            if(bitDepth < 8) { png_set_expand_gray_1_2_4_to_8(pngRead); }
            break;
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