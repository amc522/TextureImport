#include <teximp/targa/targa_importer.teximp.h>

#ifdef TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP

#include <glm/common.hpp>
#include <gpufmt/convert.h>
#include <gpufmt/traits.h>
#include <gpufmt/utility.h>

namespace teximp::targa
{
//------------------------------------------
// Reader structs
//------------------------------------------

template<class ImageColorT, class T>
struct ColorTransformResult
{
    using type = std::invoke_result_t<T, ImageColorT, const gpufmt::FormatInfo&>;
};

template<class ImageColorT>
struct ColorTransformResult<ImageColorT, std::nullptr_t>
{
    using type = ImageColorT;
};

template<class IndexT, class ImageColorT, class ColorTransformFunc>
struct ColorMapRleReader
{
    const ColorTransformFunc mColorTransformFunc;

    constexpr ColorMapRleReader(ColorTransformFunc colorTransformFunc) noexcept
        : mColorTransformFunc(colorTransformFunc)
    {}

    [[nodiscard]] int operator()(std::istream& stream, std::span<std::byte> outByteBuffer,
                                 std::span<const std::byte> colorMap, const gpufmt::FormatInfo& formatInfo) const
    {
        using OutColorT = ColorTransformResult<ImageColorT, ColorTransformFunc>::type;

        uint8_t headerByte;
        stream.read((char*)(&headerByte), 1);

        std::span outPixelBuffer = castWritableBytes<OutColorT>(outByteBuffer);
        std::span typedColorMap = castBytes<ImageColorT>(colorMap);

        const int count = glm::min((int)outPixelBuffer.size(), (int)(headerByte & 0b0111'1111) + 1);

        if((headerByte & 0b1000'0000) == 0b1000'0000)
        {
            // repeated data
            IndexT index;
            stream.read((char*)&index, sizeof(IndexT));

            const ImageColorT color = typedColorMap[index];

            if constexpr(std::is_null_pointer_v<ColorTransformFunc>)
            {
                std::fill_n(outPixelBuffer.begin(), count, color);
            }
            else
            {
                const OutColorT outColor = mColorTransformFunc(color, formatInfo);
                std::fill_n(outPixelBuffer.begin(), count, outColor);
            }
        }
        else
        {
            // uncompressed data
            for(int i = 0; i < count; ++i)
            {
                IndexT index;
                stream.read((char*)&index, sizeof(IndexT));

                const ImageColorT color = typedColorMap[index];

                if constexpr(std::is_null_pointer_v<ColorTransformFunc>) { outPixelBuffer[i] = color; }
                else { outPixelBuffer[i] = mColorTransformFunc(color, formatInfo); }
            }
        }

        return count * sizeof(OutColorT);
    }
};

template<class IndexT, class ImageColorT, class ColorTransformFunc>
struct ColorMapUncompressedReader
{
    const ColorTransformFunc mColorTransformFunc;

    constexpr ColorMapUncompressedReader(ColorTransformFunc colorTransformFunc) noexcept
        : mColorTransformFunc(colorTransformFunc)
    {}

    [[nodiscard]] int operator()(std::istream& stream, std::span<std::byte> outByteBuffer,
                                 std::span<const std::byte> colorMap, const gpufmt::FormatInfo& formatInfo) const
    {
        using OutColorT = ColorTransformResult<ImageColorT, ColorTransformFunc>::type;

        std::span outPixelBuffer = castWritableBytes<OutColorT>(outByteBuffer);
        std::span typedColorMap = castBytes<ImageColorT>(colorMap);

        for(int i = 0; i < std::ssize(outPixelBuffer); ++i)
        {
            IndexT index;
            stream.read((char*)&index, sizeof(IndexT));

            const ImageColorT color = typedColorMap[index];

            if constexpr(std::is_null_pointer_v<ColorTransformFunc>) { outPixelBuffer[i] = color; }
            else { outPixelBuffer[i] = mColorTransformFunc(color, formatInfo); }
        }

        return outPixelBuffer.size_bytes();
    }
};

template<class IndexT, class ImageColorT,
         template<class IndexT, class ImageColorT, class ColorTransformFunc> class ColorMapReader,
         class ColorTransformFunc>
[[nodiscard]] constexpr auto makeColorMapReader(ColorTransformFunc colorTransformFunc) noexcept
{
    return ColorMapReader<IndexT, ImageColorT, ColorTransformFunc>(colorTransformFunc);
}

template<class ImageColorT, class ColorTransformFunc>
struct ColorRleReader
{
    const ColorTransformFunc mColorTransformFunc;

    constexpr ColorRleReader(ColorTransformFunc colorTransformFunc) noexcept
        : mColorTransformFunc(colorTransformFunc)
    {}

    [[noexcept]] int operator()(std::istream& stream, std::span<std::byte> outByteBuffer,
                                const gpufmt::FormatInfo& formatInfo) const
    {
        using OutColorT = ColorTransformResult<ImageColorT, ColorTransformFunc>::type;

        uint8_t headerByte;
        stream.read((char*)(&headerByte), 1);

        std::span outPixelBuffer = castWritableBytes<OutColorT>(outByteBuffer);

        const int count = glm::min((int)outPixelBuffer.size(), (int)(headerByte & 0b0111'1111) + 1);

        if((headerByte & 0b1000'0000) == 0b1000'0000)
        {
            // repeated data
            ImageColorT color;
            stream.read((char*)&color, sizeof(ImageColorT));

            if constexpr(std::is_null_pointer_v<ColorTransformFunc>)
            {
                std::fill_n(outPixelBuffer.begin(), count, color);
            }
            else
            {
                OutColorT outColor = mColorTransformFunc(color, formatInfo);
                std::fill_n(outPixelBuffer.begin(), count, outColor);
            }
        }
        else
        {
            // uncompressed data
            if constexpr(std::is_null_pointer_v<ColorTransformFunc>)
            {
                stream.read((char*)outPixelBuffer.subspan(0, count).data(), count * sizeof(OutColorT));
            }
            else
            {
                for(int i = 0; i < count; ++i)
                {
                    ImageColorT color;
                    stream.read((char*)&color, sizeof(ImageColorT));

                    outPixelBuffer[i] = mColorTransformFunc(color, formatInfo);
                }
            }
        }

        return count * sizeof(OutColorT);
    }
};

template<class ImageColorT, class ColorTransformFunc>
struct ColorUncompressedReader
{
    const ColorTransformFunc mColorTransformFunc;

    constexpr ColorUncompressedReader(ColorTransformFunc colorTransformFunc) noexcept
        : mColorTransformFunc(colorTransformFunc)
    {}

    [[nodiscard]] int operator()(std::istream& stream, std::span<std::byte> outByteBuffer,
                                 const gpufmt::FormatInfo& formatInfo) const
    {
        using OutColorT = ColorTransformResult<ImageColorT, ColorTransformFunc>::type;

        std::span outPixelBuffer = castWritableBytes<OutColorT>(outByteBuffer);

        if constexpr(std::is_null_pointer_v<ColorTransformFunc>)
        {
            stream.read((char*)outPixelBuffer.data(), outPixelBuffer.size_bytes());
        }
        else
        {
            for(int i = 0; i < std::ssize(outPixelBuffer); ++i)
            {
                ImageColorT color;
                stream.read((char*)&color, sizeof(ImageColorT));

                outPixelBuffer[i] = mColorTransformFunc(color, formatInfo);
            }
        }

        return outPixelBuffer.size_bytes();
    }
};

template<class ImageColorT, template<class ImageColorT, class ColorTransformFunc> class ColorReader,
         class ColorTransformFunc>
[[nodiscard]] constexpr auto makeColorReader(ColorTransformFunc colorTransformFunc) noexcept
{
    return ColorReader<ImageColorT, ColorTransformFunc>(colorTransformFunc);
}

//-------------------------------------
// Color Transform Functions
//-------------------------------------

template<class ColorTransformFunc>
struct AlphaDiscarder
{
    ColorTransformFunc mColorTransformFunc;

    constexpr AlphaDiscarder(ColorTransformFunc colorTransformFunc) noexcept
        : mColorTransformFunc(colorTransformFunc)
    {}

    template<class T>
    [[nodiscard]] constexpr auto operator()(T imageColor, const gpufmt::FormatInfo& formatInfo) const noexcept
    {
        if constexpr(!std::is_same_v<ColorTransformFunc, std::nullptr_t>)
        {
            auto transformedColor = mColorTransformFunc(imageColor, formatInfo);
            return discardAlpha(transformedColor, formatInfo);
        }
        else { return discardAlpha(imageColor, formatInfo); }
    }

    [[nodiscard]] constexpr uint16_t discardAlpha(uint16_t imageColor,
                                                  const gpufmt::FormatInfo& formatInfo) const noexcept
    {
        return imageColor | formatInfo.alphaBitMask.mask;
    }

    [[nodiscard]] constexpr uint32_t discardAlpha(uint32_t imageColor,
                                                  const gpufmt::FormatInfo& formatInfo) const noexcept
    {
        return (formatInfo.hasChannel(gpufmt::Channel::Alpha)) ? imageColor | formatInfo.alphaBitMask.mask : imageColor;
    }
};

[[nodiscard]] constexpr glm::u8vec3 grayToRgb(uint8_t imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    return glm::u8vec3(imageColor, imageColor, imageColor);
}

[[nodiscard]] constexpr uint32_t grayToRgbx(uint8_t imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    return ((uint32_t)imageColor << formatInfo.redBitMask.offset) |
           ((uint32_t)imageColor << formatInfo.greenBitMask.offset) |
           ((uint32_t)imageColor << formatInfo.blueBitMask.offset);
}

[[nodiscard]] constexpr uint32_t grayToRgba(uint8_t imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    return ((uint32_t)imageColor << formatInfo.redBitMask.offset) |
           ((uint32_t)imageColor << formatInfo.greenBitMask.offset) |
           ((uint32_t)imageColor << formatInfo.blueBitMask.offset) | ((uint32_t)0xff << formatInfo.alphaBitMask.offset);
}

[[nodiscard]] constexpr uint16_t rgba5551Swizzle(uint16_t imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    constexpr auto& sourceInfo = gpufmt::FormatTraits<gpufmt::Format::A1R5G5B5_UNORM_PACK16>::info;

    return (((imageColor & sourceInfo.redBitMask.mask) >> sourceInfo.redBitMask.offset)
            << formatInfo.redBitMask.offset) |
           (((imageColor & sourceInfo.greenBitMask.mask) >> sourceInfo.greenBitMask.offset)
            << formatInfo.greenBitMask.offset) |
           (((imageColor & sourceInfo.blueBitMask.mask) >> sourceInfo.blueBitMask.offset)
            << formatInfo.blueBitMask.offset) |
           (((imageColor & sourceInfo.alphaBitMask.mask) >> sourceInfo.alphaBitMask.offset)
            << formatInfo.alphaBitMask.offset);
}

[[nodiscard]] constexpr glm::u8vec3 rgba5551ToRgb(uint16_t imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    constexpr auto& sourceFormatInfo = gpufmt::FormatTraits<gpufmt::Format::A1R5G5B5_UNORM_PACK16>::info;

    glm::u8vec3 outColor;
    outColor[formatInfo.redIndex] =
        gpufmt::internal::unpackBits<uint16_t, 5, sourceFormatInfo.redBitMask.offset>(imageColor);
    outColor[formatInfo.greenIndex] =
        gpufmt::internal::unpackBits<uint16_t, 5, sourceFormatInfo.greenBitMask.offset>(imageColor);
    outColor[formatInfo.blueIndex] =
        gpufmt::internal::unpackBits<uint16_t, 5, sourceFormatInfo.blueBitMask.offset>(imageColor);
    return outColor;
}

[[nodiscard]] constexpr uint32_t rgba5551ToRgba(uint16_t imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    constexpr auto& sourceFormatInfo = gpufmt::FormatTraits<gpufmt::Format::A1R5G5B5_UNORM_PACK16>::info;

    return (formatInfo.srgb) ? gpufmt::convert5551UNormTo8888SRgb(imageColor, sourceFormatInfo, formatInfo)
                             : gpufmt::convert5551UNormTo8888UNorm(imageColor, sourceFormatInfo, formatInfo);
}

[[nodiscard]] constexpr glm::u8vec3 rgbSwizzle(glm::u8vec3 imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    glm::u8vec3 outColor;
    outColor[formatInfo.redIndex] = imageColor[0];
    outColor[formatInfo.greenIndex] = imageColor[1];
    outColor[formatInfo.blueIndex] = imageColor[2];

    return outColor;
}

[[nodiscard]] constexpr uint32_t rgbToRgbaUNorm(glm::u8vec3 imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    constexpr const gpufmt::FormatInfo& sourceFormatInfo = gpufmt::FormatTraits<gpufmt::Format::B8G8R8_UNORM>::info;

    return gpufmt::convert888UNormTo8888Or888XUNorm(imageColor, sourceFormatInfo, formatInfo);
}

[[nodiscard]] constexpr uint32_t rgbToRgbaSRgb(glm::u8vec3 imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    constexpr const gpufmt::FormatInfo& sourceFormatInfo = gpufmt::FormatTraits<gpufmt::Format::B8G8R8_SRGB>::info;

    return gpufmt::convert888SRgbTo8888Or888XSRgb(imageColor, sourceFormatInfo, formatInfo);
}

[[nodiscard]] constexpr uint32_t rgbaSwizzle(uint32_t imageColor, const gpufmt::FormatInfo& formatInfo) noexcept
{
    constexpr auto& sourceFormatInfo = gpufmt::FormatTraits<gpufmt::Format::R8G8B8A8_UNORM>::info;

    uint32_t outColor = (gpufmt::internal::unpackBits<uint32_t, 8, sourceFormatInfo.redBitMask.offset>(imageColor)
                         << formatInfo.redBitMask.offset) |
                        (gpufmt::internal::unpackBits<uint32_t, 8, sourceFormatInfo.greenBitMask.offset>(imageColor)
                         << formatInfo.greenBitMask.offset) |
                        (gpufmt::internal::unpackBits<uint32_t, 8, sourceFormatInfo.blueBitMask.offset>(imageColor)
                         << formatInfo.blueBitMask.offset);

    outColor |= (formatInfo.hasChannel(gpufmt::Channel::Alpha))
                    ? (gpufmt::internal::unpackBits<uint32_t, 8, sourceFormatInfo.blueBitMask.offset>(imageColor)
                       << formatInfo.blueBitMask.offset)
                    : 0u;

    return outColor;
}

[[nodiscard]] constexpr gpufmt::Format getImageFormat(int bitsPerPixel, bool srgb, bool keepAlpha) noexcept
{
    if(bitsPerPixel == 15 || bitsPerPixel == 16) { return gpufmt::Format::A1R5G5B5_UNORM_PACK16; }
    else if(bitsPerPixel == 24) { return (srgb) ? gpufmt::Format::B8G8R8_SRGB : gpufmt::Format::B8G8R8_UNORM; }
    else if(bitsPerPixel == 32) { return (srgb) ? gpufmt::Format::B8G8R8A8_SRGB : gpufmt::Format::B8G8R8A8_UNORM; }

    return gpufmt::Format::UNDEFINED;
}

template<class Pred>
constexpr void visitColorTransform(cputex::SurfaceSpan surface, uint8_t bitsPerPixel, bool keepAlpha,
                                   Pred pred) noexcept
{
    const gpufmt::FormatInfo& destFormatInfo = gpufmt::formatInfo(surface.format());
    const gpufmt::Format sourceFormat = getImageFormat(bitsPerPixel, destFormatInfo.srgb, keepAlpha);

    if(sourceFormat == surface.format() && keepAlpha)
    {
        if(destFormatInfo.blockByteSize == 2) // 5551
        {
            pred(uint16_t(), nullptr);
        }
        else if(destFormatInfo.blockByteSize == 3) // 888
        {
            pred(glm::u8vec3(), nullptr);
        }
        else if(destFormatInfo.blockByteSize == 4) // 8888
        {
            pred(uint32_t(), nullptr);
        }
    }
    else if(sourceFormat == surface.format() && !keepAlpha)
    {
        if(destFormatInfo.blockByteSize == 2) // 5551
        {
            pred(uint16_t(), AlphaDiscarder(nullptr));
        }
        else if(destFormatInfo.blockByteSize == 3) // 888
        {
            pred(glm::u8vec3(), nullptr);
        }
        else if(destFormatInfo.blockByteSize == 4) // 8888
        {
            pred(uint32_t(), AlphaDiscarder(nullptr));
        }
    }
    else if(sourceFormat == gpufmt::Format::A1R5G5B5_UNORM_PACK16 && keepAlpha)
    {
        if(gpufmt::isFormat5551UNorm(surface.format())) { pred(uint16_t(), rgba5551Swizzle); }
        else if(gpufmt::isFormat888UNorm(surface.format())) { pred(glm::uint16_t(), rgba5551ToRgb); }
        else if(gpufmt::isFormat8888Or888XUNorm(surface.format())) { pred(uint16_t(), rgba5551ToRgba); }
    }
    else if(sourceFormat == gpufmt::Format::A1R5G5B5_UNORM_PACK16 && !keepAlpha)
    {
        if(gpufmt::isFormat5551UNorm(surface.format())) { pred(uint16_t(), AlphaDiscarder(rgba5551Swizzle)); }
        else if(gpufmt::isFormat888UNorm(surface.format())) { pred(glm::uint16_t(), rgba5551ToRgb); }
        else if(gpufmt::isFormat8888Or888XUNorm(surface.format())) { pred(uint16_t(), AlphaDiscarder(rgba5551ToRgba)); }
    }
    else if(sourceFormat == gpufmt::Format::B8G8R8_UNORM || sourceFormat == gpufmt::Format::B8G8R8_SRGB)
    {
        if(gpufmt::isFormat888UNorm(surface.format())) { pred(glm::u8vec3(), rgbSwizzle); }
        else if(gpufmt::isFormat8888Or888XSRgb(surface.format())) { pred(glm::u8vec3(), rgbToRgbaSRgb); }
        else if(gpufmt::isFormat8888Or888XUNorm(surface.format())) { pred(glm::u8vec3(), rgbToRgbaUNorm); }
    }
    else if((sourceFormat == gpufmt::Format::B8G8R8A8_UNORM || sourceFormat == gpufmt::Format::B8G8R8A8_SRGB) &&
            (gpufmt::isFormat8888Or888XUNorm(surface.format()) || gpufmt::isFormat8888Or888XSRgb(surface.format())))
    {
        if(keepAlpha) { pred(uint32_t(), rgbaSwizzle); }
        else { pred(uint32_t(), AlphaDiscarder(rgbaSwizzle)); }
    }
}

//----------------------------
// Read functions
//----------------------------
template<class ReadFunc>
void readUpperLeft(std::istream& stream, cputex::SurfaceSpan surface, ReadFunc readFunc)
{
    const gpufmt::FormatInfo& formatInfo = gpufmt::formatInfo(surface.format());
    std::span surfaceData = surface.accessData();

    while(!surfaceData.empty())
    {
        const int bytesWritten = readFunc(stream, surfaceData, formatInfo);
        surfaceData = surfaceData.subspan(bytesWritten);
    }
}

template<class ReadFunc>
void readLowerLeft(std::istream& stream, cputex::SurfaceSpan surface, ReadFunc readFunc)
{
    const gpufmt::FormatInfo& formatInfo = gpufmt::formatInfo(surface.format());
    std::span surfaceData = surface.accessData();
    const cputex::Extent& extent = surface.extent();

    for(int y = extent.y - 1; y >= 0; --y)
    {
        std::span surfaceRowData =
            surfaceData.subspan(y * extent.x * formatInfo.blockByteSize, extent.x * formatInfo.blockByteSize);

        while(!surfaceRowData.empty())
        {
            const int bytesWritten = readFunc(stream, surfaceRowData, formatInfo);
            surfaceRowData = surfaceRowData.subspan(bytesWritten);
        }
    }
}

template<class IndexT, class ImageColorT, class ColorTransformT,
         template<class IndexT, class ImageColorT, class ColorTransformT> class ColorMapReader>
void readColorMapOriginSelect(std::istream& stream, cputex::SurfaceSpan surface, std::span<const std::byte> colorMap,
                              TargaTexImpImporter::ImageOrigin imageOrigin,
                              ColorMapReader<IndexT, ImageColorT, ColorTransformT> colorMapReader)
{
    if(imageOrigin == TargaTexImpImporter::ImageOrigin::UpperLeft)
    {
        readUpperLeft(stream, surface,
                      [&](std::istream& stream, std::span<std::byte> outBuffer, const gpufmt::FormatInfo& formatInfo)
                      { return colorMapReader(stream, outBuffer, colorMap, formatInfo); });
    }
    else if(imageOrigin == TargaTexImpImporter::ImageOrigin::LowerLeft)
    {
        readLowerLeft(stream, surface,
                      [&](std::istream& stream, std::span<std::byte> outBuffer, const gpufmt::FormatInfo& formatInfo)
                      { return colorMapReader(stream, outBuffer, colorMap, formatInfo); });
    }
}

template<class IndexT, template<class IndexT, class ImageColorT, class ColorTransformFunc> class ColorMapReader>
void readColorMapColorTransformSelect(std::istream& stream, cputex::SurfaceSpan surface,
                                      std::span<const std::byte> colorMap, TargaTexImpImporter::ImageOrigin imageOrigin,
                                      int colorMapEntryBitSize, bool keepAlpha)
{
    visitColorTransform(
        surface, colorMapEntryBitSize, keepAlpha,
        [&](auto imageValueTypePlaceholder, auto colorTransform)
        {
            readColorMapOriginSelect(
                stream, surface, colorMap, imageOrigin,
                makeColorMapReader<IndexT, decltype(imageValueTypePlaceholder), ColorMapReader>(colorTransform));
        });
}

template<template<class IndexT, class ImageColorT, class ColorTransformFunc> class ColorMapReader>
void readColorMapIndexTypeSelect(std::istream& stream, cputex::SurfaceSpan surface, std::span<const std::byte> colorMap,
                                 TargaTexImpImporter::ImageOrigin imageOrigin, int bitsPerPixel,
                                 int colorMapEntryBitSize, bool keepAlpha)
{
    if(bitsPerPixel == 8)
    {
        readColorMapColorTransformSelect<uint8_t, ColorMapReader>(stream, surface, colorMap, imageOrigin,
                                                                  colorMapEntryBitSize, keepAlpha);
    }
    else if(bitsPerPixel == 16)
    {
        readColorMapColorTransformSelect<uint16_t, ColorMapReader>(stream, surface, colorMap, imageOrigin,
                                                                   colorMapEntryBitSize, keepAlpha);
    }
}

template<class ImageColorT, class ColorTransformT, template<class ImageColorT, class ColorTransformT> class ColorReader>
void readColorOriginSelect(std::istream& stream, cputex::SurfaceSpan surface,
                           TargaTexImpImporter::ImageOrigin imageOrigin,
                           ColorReader<ImageColorT, ColorTransformT> colorReader)
{
    if(imageOrigin == TargaTexImpImporter::ImageOrigin::UpperLeft)
    {
        readUpperLeft(stream, surface,
                      [&](std::istream& stream, std::span<std::byte> outBuffer, const gpufmt::FormatInfo& formatInfo)
                      { return colorReader(stream, outBuffer, formatInfo); });
    }
    else if(imageOrigin == TargaTexImpImporter::ImageOrigin::LowerLeft)
    {
        readLowerLeft(stream, surface,
                      [&](std::istream& stream, std::span<std::byte> outBuffer, const gpufmt::FormatInfo& formatInfo)
                      { return colorReader(stream, outBuffer, formatInfo); });
    }
}

template<template<class ImageColorT, class ColorTransformFunc> class ColorReader>
void readColorColorTransformSelect(std::istream& stream, cputex::SurfaceSpan surface,
                                   TargaTexImpImporter::ImageOrigin imageOrigin, uint8_t bitsPerPixel, bool keepAlpha)
{
    visitColorTransform(surface, bitsPerPixel, keepAlpha,
                        [&](auto imageValueTypePlaceholder, auto colorTransform)
                        {
                            readColorOriginSelect(
                                stream, surface, imageOrigin,
                                makeColorReader<decltype(imageValueTypePlaceholder), ColorReader>(colorTransform));
                        });
}

template<template<class ImageColorT, class ColorTransformFunc> class ColorReader>
void readGrayScaleColorTransformSelect(std::istream& stream, cputex::SurfaceSpan surface,
                                       TargaTexImpImporter::ImageOrigin imageOrigin, bool keepAlpha)
{
    const gpufmt::FormatInfo& formatInfo = gpufmt::formatInfo(surface.format());

    if(formatInfo.blockByteSize == 3)
    {
        readColorOriginSelect(stream, surface, imageOrigin, makeColorReader<uint8_t, ColorReader>(grayToRgb));
    }
    else if(formatInfo.blockByteSize == 4 && formatInfo.alphaBitMask.width == 0)
    {
        readColorOriginSelect(stream, surface, imageOrigin, makeColorReader<uint8_t, ColorReader>(grayToRgbx));
    }
    else if(formatInfo.blockByteSize == 4)
    {
        readColorOriginSelect(stream, surface, imageOrigin, makeColorReader<uint8_t, ColorReader>(grayToRgba));
    }
}

//---------------------------
// TargaTexImpImporter
//---------------------------

FileFormat TargaTexImpImporter::fileFormat() const
{
    return FileFormat::Targa;
}

bool TargaTexImpImporter::checkSignature(std::istream& stream)
{
    stream.seekg(-26, std::ios_base::end);

    stream.read((char*)(&mFooter.extensionOffset), 4);
    stream.read((char*)(&mFooter.developerAreaOffset), 4);
    stream.read(mFooter.signature.data(), mFooter.signature.size());
    stream.read(mFooter.end.data(), mFooter.end.size());

    std::array<char, 16> tgaSignature = {'T', 'R', 'U', 'E', 'V', 'I', 'S', 'I',
                                         'O', 'N', '-', 'X', 'F', 'I', 'L', 'E'};
    std::array<char, 2> footerEnd = {'.', 0};
    if(mFooter.signature == tgaSignature && mFooter.end == footerEnd)
    {
        mFooterFound = true;
        return true;
    }

    return readBaseHeader(stream);
}

template<class T>
void readField(std::istream& stream, T& object)
{
    stream.read((char*)&object, sizeof(object));
}

bool TargaTexImpImporter::readBaseHeader(std::istream& stream)
{
    stream.seekg(0);

    readField(stream, mHeader.idLength);
    readField(stream, mHeader.colorMapType);
    readField(stream, mHeader.imageType);

    if(!((mHeader.imageType >= 0 && mHeader.imageType <= 3) || (mHeader.imageType >= 9 && mHeader.imageType <= 11)))
    {
        setError(TextureImportError::InvalidDataInImage, std::format("Unknown imagee type {}", mHeader.imageType));
        return false;
    }

    readField(stream, mHeader.colorMap.firstEntryIndex);
    readField(stream, mHeader.colorMap.length);
    readField(stream, mHeader.colorMap.entrySize);

    if(mHeader.colorMapType == 1)
    {
        if(mHeader.colorMap.entrySize != 8 && mHeader.colorMap.entrySize != 15 && mHeader.colorMap.entrySize != 16 &&
           mHeader.colorMap.entrySize != 24 && mHeader.colorMap.entrySize != 32)
        {
            setError(TextureImportError::InvalidDataInImage,
                     std::format("Invalid color map entry bit size {}", mHeader.colorMap.entrySize));
            return false;
        }
    }

    readField(stream, mHeader.image.xOrigin);
    readField(stream, mHeader.image.yOrigin);
    readField(stream, mHeader.image.width);

    if(mHeader.image.width < 1)
    {
        setError(TextureImportError::InvalidDataInImage, "Image has width of 0");
        return false;
    }

    readField(stream, mHeader.image.height);

    if(mHeader.image.height < 1)
    {
        setError(TextureImportError::InvalidDataInImage, "Image has height of 0");
        return false;
    }

    if(mHeader.image.width > kMaxTextureWidth || mHeader.image.height > kMaxTextureHeight)
    {
        setError(TextureImportError::DimensionsTooLarge,
                 std::format("Image has size {}x{}. Max supported dimensions are {}x{}.", mHeader.image.width,
                             mHeader.image.height, kMaxTextureWidth, kMaxTextureHeight));
        return false;
    }

    readField(stream, mHeader.image.bitsPerPixel);
    readField(stream, mHeader.image.descriptor);

    if(mHeader.colorMapType == 1 && (mHeader.image.bitsPerPixel != 8) && (mHeader.image.bitsPerPixel != 16))
    {
        setError(TextureImportError::InvalidDataInImage,
                 std::format("Invalid color map index bit size {}", mHeader.image.bitsPerPixel));
        return false;
    }

    if(mHeader.image.bitsPerPixel != 8 && mHeader.image.bitsPerPixel != 15 && mHeader.image.bitsPerPixel != 16 &&
       mHeader.image.bitsPerPixel != 24 && mHeader.image.bitsPerPixel != 32)
    {
        setError(TextureImportError::InvalidDataInImage,
                 std::format("Invalid pixel bit size {}", mHeader.image.bitsPerPixel));
        return false;
    }

    mImageOrigin = (ImageOrigin)mHeader.image.origin;

    if(mImageOrigin == ImageOrigin::LowerRight)
    {
        setError(TextureImportError::UnsupportedFeature, "Image origin in the lower right is not supported");
        return false;
    }

    if(mImageOrigin == ImageOrigin::UpperRight)
    {
        setError(TextureImportError::UnsupportedFeature, "Image origin in the upper right is not supported");
        return false;
    }

    return true;
}

void TargaTexImpImporter::readExtensionHeader(std::istream& stream)
{
    std::streampos currentStreamPos = stream.tellg();
    stream.seekg(mFooter.extensionOffset, std::ios::beg);

    readField(stream, mExtensionHeader.extensionByteSize);

    if(mExtensionHeader.extensionByteSize != 495)
    {
        stream.seekg(currentStreamPos, std::ios::beg);
        return;
    }

    readField(stream, mExtensionHeader.authorNameBuffer);
    readField(stream, mExtensionHeader.authorCommentBuffers);
    readField(stream, mExtensionHeader.dateTimeStamp);
    readField(stream, mExtensionHeader.jobId);
    readField(stream, mExtensionHeader.jobTime);
    readField(stream, mExtensionHeader.softwareId);
    readField(stream, mExtensionHeader.softwareVersion);
    readField(stream, mExtensionHeader.keyColor);
    readField(stream, mExtensionHeader.pixelAspectRatio);
    readField(stream, mExtensionHeader.gammaValue);
    readField(stream, mExtensionHeader.colorCorrectionOffset);
    readField(stream, mExtensionHeader.postageStampOffset);
    readField(stream, mExtensionHeader.scanLineOffset);
    readField(stream, mExtensionHeader.attributesType);

    stream.seekg(currentStreamPos, std::ios::beg);
}

gpufmt::Format TargaTexImpImporter::queryFormatForGrayScaleImage(ITextureAllocator& textureAllocator,
                                                                 TextureImportOptions options)
{
    return queryFormatForRgbImage(textureAllocator, options);
}

gpufmt::Format TargaTexImpImporter::queryFormatFor555xImage(ITextureAllocator& textureAllocator,
                                                            TextureImportOptions options)
{
    FormatLayout selectedLayout;

    if(!options.padRgbWithAlpha)
    {
        constexpr FormatLayout nativeLayout = FormatLayout::_5_5_5_1;
        constexpr std::array additionalLayouts = {FormatLayout::_8_8_8, FormatLayout::_8_8_8_8};

        selectedLayout = textureAllocator.selectFormatLayout(nativeLayout, additionalLayouts);

        if(!isValidFormatLayout(nativeLayout, std::span(additionalLayouts), selectedLayout))
        {
            setTextureAllocatorFormatLayoutError(selectedLayout);
            return gpufmt::Format::UNDEFINED;
        }
    }
    else
    {
        constexpr FormatLayout nativeLayout = FormatLayout::_5_5_5_1;
        constexpr std::array additionalLayouts = {FormatLayout::_8_8_8_8};

        selectedLayout = textureAllocator.selectFormatLayout(nativeLayout, additionalLayouts);

        if(!isValidFormatLayout(nativeLayout, std::span(additionalLayouts), selectedLayout))
        {
            setTextureAllocatorFormatLayoutError(selectedLayout);
            return gpufmt::Format::UNDEFINED;
        }
    }

    std::span<const gpufmt::Format> availableFormats;

    if(selectedLayout == FormatLayout::_5_5_5_1)
    {
        constexpr std::array formats = {gpufmt::Format::A1R5G5B5_UNORM_PACK16, gpufmt::Format::R5G5B5A1_UNORM_PACK16,
                                        gpufmt::Format::B5G5R5A1_UNORM_PACK16};
        availableFormats = formats;
    }
    else if(selectedLayout == FormatLayout::_8_8_8)
    {
        constexpr std::array unormFormats = {gpufmt::Format::B8G8R8_UNORM, gpufmt::Format::R8G8B8_UNORM};
        constexpr std::array sRgbFormats = {gpufmt::Format::B8G8R8_SRGB, gpufmt::Format::R8G8B8_SRGB};

        availableFormats = (options.assumeSrgb) ? sRgbFormats : unormFormats;
    }
    else if(selectedLayout == FormatLayout::_8_8_8_8)
    {
        constexpr std::array unormFormats = {gpufmt::Format::B8G8R8X8_UNORM, gpufmt::Format::B8G8R8A8_UNORM,
                                             gpufmt::Format::R8G8B8A8_UNORM, gpufmt::Format::A8B8G8R8_UNORM_PACK32};
        constexpr std::array sRgbFormats = {gpufmt::Format::B8G8R8X8_SRGB, gpufmt::Format::B8G8R8A8_SRGB,
                                            gpufmt::Format::R8G8B8A8_SRGB, gpufmt::Format::A8B8G8R8_SRGB_PACK32};

        availableFormats = (options.assumeSrgb) ? sRgbFormats : unormFormats;
    }
    else
    {
        setTextureAllocatorFormatLayoutError(selectedLayout);
        return gpufmt::Format::UNDEFINED;
    }

    const gpufmt::Format format = textureAllocator.selectFormat(selectedLayout, availableFormats);

    if(!contains(availableFormats, format))
    {
        setTextureAllocatorFormatError(format);
        return gpufmt::Format::UNDEFINED;
    }

    return format;
}

gpufmt::Format TargaTexImpImporter::queryFormatFor5551Image(ITextureAllocator& textureAllocator,
                                                            TextureImportOptions options)
{
    FormatLayout selectedLayout;

    constexpr FormatLayout nativeLayout = FormatLayout::_5_5_5_1;
    constexpr std::array additionalLayouts = {FormatLayout::_8_8_8_8};

    selectedLayout = textureAllocator.selectFormatLayout(nativeLayout, additionalLayouts);

    if(!isValidFormatLayout(nativeLayout, std::span(additionalLayouts), selectedLayout))
    {
        setTextureAllocatorFormatLayoutError(selectedLayout);
        return gpufmt::Format::UNDEFINED;
    }

    std::span<const gpufmt::Format> availableFormats;

    if(selectedLayout == FormatLayout::_5_5_5_1)
    {
        constexpr std::array formats = {gpufmt::Format::A1R5G5B5_UNORM_PACK16, gpufmt::Format::R5G5B5A1_UNORM_PACK16,
                                        gpufmt::Format::B5G5R5A1_UNORM_PACK16};
        availableFormats = formats;
    }
    else if(selectedLayout == FormatLayout::_8_8_8_8)
    {
        constexpr std::array unormFormats = {gpufmt::Format::B8G8R8A8_UNORM, gpufmt::Format::R8G8B8A8_UNORM,
                                             gpufmt::Format::A8B8G8R8_UNORM_PACK32};
        constexpr std::array sRgbFormats = {gpufmt::Format::B8G8R8A8_SRGB, gpufmt::Format::R8G8B8A8_SRGB,
                                            gpufmt::Format::A8B8G8R8_SRGB_PACK32};

        availableFormats = (options.assumeSrgb) ? sRgbFormats : unormFormats;
    }
    else
    {
        setTextureAllocatorFormatLayoutError(selectedLayout);
        return gpufmt::Format::UNDEFINED;
    }

    const gpufmt::Format format = textureAllocator.selectFormat(selectedLayout, availableFormats);

    if(!contains(availableFormats, format))
    {
        setTextureAllocatorFormatError(format);
        return gpufmt::Format::UNDEFINED;
    }

    return format;
}

gpufmt::Format TargaTexImpImporter::queryFormatForRgbImage(ITextureAllocator& textureAllocator,
                                                           TextureImportOptions options)
{
    FormatLayout selectedLayout;

    if(!options.padRgbWithAlpha)
    {
        constexpr FormatLayout nativeLayout = FormatLayout::_8_8_8;
        constexpr std::array additionalLayouts = {FormatLayout::_8_8_8_8};

        selectedLayout = textureAllocator.selectFormatLayout(nativeLayout, additionalLayouts);

        if(!isValidFormatLayout(nativeLayout, std::span(additionalLayouts), selectedLayout))
        {
            setTextureAllocatorFormatLayoutError(selectedLayout);
            return gpufmt::Format::UNDEFINED;
        }
    }
    else { selectedLayout = FormatLayout::_8_8_8_8; }

    if(selectedLayout == FormatLayout::_8_8_8)
    {
        constexpr std::array unormFormats = {gpufmt::Format::B8G8R8_UNORM, gpufmt::Format::R8G8B8_UNORM};
        constexpr std::array sRgbFormats = {gpufmt::Format::B8G8R8_SRGB, gpufmt::Format::R8G8B8_SRGB};

        gpufmt::Format selectedFormat;

        if(options.assumeSrgb)
        {
            selectedFormat = textureAllocator.selectFormat(FormatLayout::_8_8_8, sRgbFormats);

            if(!contains(unormFormats, selectedFormat))
            {
                setTextureAllocatorFormatError(selectedFormat);
                return gpufmt::Format::UNDEFINED;
            }
        }
        else
        {
            selectedFormat = textureAllocator.selectFormat(FormatLayout::_8_8_8, unormFormats);

            if(!contains(sRgbFormats, selectedFormat))
            {
                setTextureAllocatorFormatError(selectedFormat);
                return gpufmt::Format::UNDEFINED;
            }
        }

        return selectedFormat;
    }
    else { return queryFormatForRgbxImage(textureAllocator, options); }
}

gpufmt::Format TargaTexImpImporter::queryFormatForRgbxImage(ITextureAllocator& textureAllocator,
                                                            TextureImportOptions options)
{
    std::span<const gpufmt::Format> availableFormats;

    if(options.assumeSrgb)
    {
        constexpr std::array sRgbFormats = {gpufmt::Format::B8G8R8X8_SRGB, gpufmt::Format::B8G8R8A8_SRGB,
                                            gpufmt::Format::R8G8B8A8_SRGB, gpufmt::Format::A8B8G8R8_SRGB_PACK32};
        availableFormats = sRgbFormats;
    }
    else
    {
        constexpr std::array unormFormats = {gpufmt::Format::B8G8R8X8_UNORM, gpufmt::Format::B8G8R8A8_UNORM,
                                             gpufmt::Format::R8G8B8A8_UNORM, gpufmt::Format::A8B8G8R8_UNORM_PACK32};
        availableFormats = unormFormats;
    }

    const gpufmt::Format selectedFormat = textureAllocator.selectFormat(FormatLayout::_8_8_8_8, availableFormats);

    if(!contains(availableFormats, selectedFormat))
    {
        setTextureAllocatorFormatError(selectedFormat);
        return gpufmt::Format::UNDEFINED;
    }

    return selectedFormat;
}

gpufmt::Format TargaTexImpImporter::queryFormatForRgbaImage(ITextureAllocator& textureAllocator,
                                                            TextureImportOptions options)
{
    std::span<const gpufmt::Format> availableFormats;

    if(options.assumeSrgb)
    {
        constexpr std::array sRgbFormats = {gpufmt::Format::B8G8R8A8_SRGB, gpufmt::Format::R8G8B8A8_SRGB,
                                            gpufmt::Format::A8B8G8R8_SRGB_PACK32};
        availableFormats = sRgbFormats;
    }
    else
    {
        constexpr std::array unormFormats = {gpufmt::Format::B8G8R8A8_UNORM, gpufmt::Format::R8G8B8A8_UNORM,
                                             gpufmt::Format::A8B8G8R8_UNORM_PACK32};
        availableFormats = unormFormats;
    }

    const gpufmt::Format selectedFormat = textureAllocator.selectFormat(FormatLayout::_8_8_8_8, availableFormats);

    if(!contains(availableFormats, selectedFormat))
    {
        setTextureAllocatorFormatError(selectedFormat);
        return gpufmt::Format::UNDEFINED;
    }

    return selectedFormat;
}

[[nodiscard]] bool isColorMap(uint8_t imageType)
{
    return imageType == 1 || imageType == 9;
}

[[nodiscard]] bool isTrueColor(uint8_t imageType)
{
    return imageType == 2 || imageType == 10;
}

[[nodiscard]] bool isGreyScale(uint8_t imageType)
{
    return imageType == 3 || imageType == 11;
}

[[nodiscard]] bool isRLECompressed(uint8_t imageType)
{
    return imageType >= 9 && imageType <= 11;
}

void TargaTexImpImporter::load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options)
{
    if(mFooterFound)
    {
        if(!readBaseHeader(stream)) { return; }

        if(mFooter.extensionOffset > 0) { readExtensionHeader(stream); }
    }

    if(mHeader.idLength > 0)
    {
        mImageId.resize(mHeader.idLength);
        stream.read((char*)(mImageId.data()), mHeader.idLength);
    }

    if(mHeader.colorMapType == 1)
    {
        mColorMapData.resize(mHeader.colorMap.length * (mHeader.colorMap.entrySize / 8));
        stream.read((char*)(mColorMapData.data()), mColorMapData.size());
    }

    if(mHeader.imageType == 0)
    {
        setError(TextureImportError::InvalidDataInImage, "Not image data in file.");
        return;
    }

    if(mHeader.imageType != 1 &&  // uncompressed color map
       mHeader.imageType != 2 &&  // uncompressed true color
       mHeader.imageType != 3 &&  // uncompressed grayscale
       mHeader.imageType != 9 &&  // RLE color map
       mHeader.imageType != 10 && // RLE true color
       mHeader.imageType != 11)   // RLE grayscale
    {
        setError(TextureImportError::InvalidDataInImage,
                 std::format("Image type {} is not a valid image type.", mHeader.imageType));
        return;
    }

    const uint32_t colorBitsPerPixel =
        (isColorMap(mHeader.imageType)) ? mHeader.colorMap.entrySize : mHeader.image.bitsPerPixel;

    gpufmt::Format selectedFormat = gpufmt::Format::UNDEFINED;

    if(colorBitsPerPixel == 8) { selectedFormat = queryFormatForGrayScaleImage(textureAllocator, options); }
    else if(colorBitsPerPixel == 15) { selectedFormat = queryFormatFor555xImage(textureAllocator, options); }
    else if(colorBitsPerPixel == 16 && !options.padRgbWithAlpha && !hasAlpha())
    {
        selectedFormat = queryFormatFor555xImage(textureAllocator, options);
    }
    else if(colorBitsPerPixel == 16) { selectedFormat = queryFormatFor5551Image(textureAllocator, options); }
    else if(colorBitsPerPixel == 24 && !options.padRgbWithAlpha)
    {
        selectedFormat = queryFormatForRgbImage(textureAllocator, options);
    }
    else if(colorBitsPerPixel == 24 && options.padRgbWithAlpha)
    {
        selectedFormat = queryFormatForRgbxImage(textureAllocator, options);
    }
    else if(colorBitsPerPixel == 32 && !options.padRgbWithAlpha && !hasAlpha())
    {
        selectedFormat = queryFormatForRgbxImage(textureAllocator, options);
    }
    else if(colorBitsPerPixel == 32) { selectedFormat = queryFormatForRgbaImage(textureAllocator, options); }

    if(selectedFormat == gpufmt::Format::UNDEFINED) { return; }

    cputex::TextureParams textureParams{
        .format = selectedFormat,
        .dimension = cputex::TextureDimension::Texture2D,
        .extent = {mHeader.image.width, mHeader.image.height, 1},
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

    textureAllocator.postAllocation();

    cputex::SurfaceSpan surface(textureParams.format, textureParams.dimension, textureParams.extent,
                                textureAllocator.accessTextureData(0, {}));

    // read image
    switch(mHeader.imageType)
    {
    case 1:
        // uncompressed color map
        readColorMap(stream, surface);
        break;
    case 2:
        // uncompressed true color
        readTrueColor(stream, surface);
        break;
    case 3:
        // uncompressed grayscale
        readGrayScale(stream, surface);
        break;
    case 9:
        // RLE color map
        readColorMapRLE(stream, surface);
        break;
    case 10:
        // RLE true color
        readTrueColorRLE(stream, surface);
        break;
    case 11:
        // RLE grayscale
        readGrayScaleRLE(stream, surface);
        break;
    }
}

bool TargaTexImpImporter::hasAlpha() const
{
    return mHeader.image.alphaChannelBits > 0 ||
           (mFooterFound && mExtensionHeader.attributesType != TgaAlphaType::NoAlpha &&
            mExtensionHeader.attributesType != TgaAlphaType::IgnoreUndefinedAlpha);
}

bool TargaTexImpImporter::keepAlphaValue() const
{
    if(mFooterFound && mFooter.extensionOffset > 0)
    {
        return // mExtensionHeader.attributesType == TgaAlphaType::KeepUndefinedAlpha ||
            mExtensionHeader.attributesType == TgaAlphaType::Alpha ||
            mExtensionHeader.attributesType == TgaAlphaType::PreMultAlpha;
    }
    else { return mHeader.image.alphaChannelBits > 0; }
}

void TargaTexImpImporter::readGrayScale(std::istream& stream, cputex::SurfaceSpan surface)
{
    teximp::targa::readGrayScaleColorTransformSelect<ColorUncompressedReader>(stream, surface, mImageOrigin,
                                                                              keepAlphaValue());
}

void TargaTexImpImporter::readGrayScaleRLE(std::istream& stream, cputex::SurfaceSpan surface)
{
    teximp::targa::readGrayScaleColorTransformSelect<ColorRleReader>(stream, surface, mImageOrigin, keepAlphaValue());
}

void TargaTexImpImporter::readTrueColorRLE(std::istream& stream, cputex::SurfaceSpan surface)
{
    readColorColorTransformSelect<ColorRleReader>(stream, surface, mImageOrigin, mHeader.image.bitsPerPixel,
                                                  keepAlphaValue());
}

void TargaTexImpImporter::readTrueColor(std::istream& stream, cputex::SurfaceSpan surface)
{
    readColorColorTransformSelect<ColorUncompressedReader>(stream, surface, mImageOrigin, mHeader.image.bitsPerPixel,
                                                           keepAlphaValue());
}

void TargaTexImpImporter::readColorMap(std::istream& stream, cputex::SurfaceSpan surface)
{
    teximp::targa::readColorMapIndexTypeSelect<ColorMapUncompressedReader>(
        stream, surface, mColorMapData, mImageOrigin, mHeader.image.bitsPerPixel, mHeader.colorMap.entrySize,
        keepAlphaValue());
}

void TargaTexImpImporter::readColorMapRLE(std::istream& stream, cputex::SurfaceSpan surface)
{
    teximp::targa::readColorMapIndexTypeSelect<ColorMapRleReader>(stream, surface, mColorMapData, mImageOrigin,
                                                                  mHeader.image.bitsPerPixel,
                                                                  mHeader.colorMap.entrySize, keepAlphaValue());
}
} // namespace teximp::targa

#endif // TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP