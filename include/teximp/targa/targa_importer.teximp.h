#pragma once

#include <teximp/teximp.h>

#ifdef TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP

namespace teximp::targa
{
struct TgaColorMapSpec
{
    uint16_t firstEntryIndex;
    uint16_t length;
    uint8_t entrySize;
};

struct TgaImageSpec
{
    uint16_t xOrigin = 0;
    uint16_t yOrigin = 0;
    uint16_t width = 0;
    uint16_t height = 0;
    uint8_t bitsPerPixel = 0;
    union
    {
        struct
        {
            uint8_t alphaChannelBits : 4;
            uint8_t origin : 2;
            uint8_t unused : 2;
        };
        uint8_t descriptor = 0;
    };
};

struct TgaHeader
{
    uint8_t idLength = 0;
    // color map type
    //  0: no color map
    //  1: present
    //  2 - 127: reserved
    //  128 - 255: user data
    uint8_t colorMapType = 0;
    // image type
    //  0: no image data
    //  1: uncompressed color mapped image
    //  2: uncompresssed true color image
    //  3: uncompressed grayscale
    //  9: RLE color mapped
    //  10: RLE true color
    //  11: RLE grayscale
    uint8_t imageType = 0;
    TgaColorMapSpec colorMap;
    TgaImageSpec image;
};

struct TgaFooter
{
    uint32_t extensionOffset;
    uint32_t developerAreaOffset;
    std::array<char, 16> signature;
    std::array<char, 2> end;
};

struct TgaDateTime
{
    uint16_t month = 0;
    uint16_t day = 0;
    uint16_t year = 0;
    uint16_t hour = 0;
    uint16_t minute = 0;
    uint16_t second = 0;
};
static_assert(sizeof(TgaDateTime) == 12);

struct TgaJobTime
{
    uint16_t hours = 0;
    uint16_t minutes = 0;
    uint16_t seconds = 0;
};
static_assert(sizeof(TgaJobTime) == 6);

struct TgaKeyColor
{
    uint8_t blue = 0;
    uint8_t green = 0;
    uint8_t red = 0;
    uint8_t alpha = 0;
};
static_assert(sizeof(TgaKeyColor) == 4);

struct TgaRatio
{
    uint16_t numerator = 0;
    uint16_t denominator = 0;
};
static_assert(sizeof(TgaRatio) == 4);

enum class TgaAlphaType : uint8_t
{
    NoAlpha = 0,              // no Alpha data included (bits 3-0 of image descriptor field should also be set to zero)
    IgnoreUndefinedAlpha = 1, // undefined data in the Alpha field, can be ignored
    KeepUndefinedAlpha = 2,   // undefined data in the Alpha field, but should be retained
    Alpha = 3,
    PreMultAlpha = 4,
};
static_assert(sizeof(TgaAlphaType) == 1);

struct TgaExtensionHeader
{
    // Size in bytes of the extension area, always 495
    uint16_t extensionByteSize = 0;
    // Name of the author.If not used, bytes should be set to NULL(\0) or spaces
    std::array<char, 41> authorNameBuffer;
    // A comment, organized as four lines, each consisting of 80 characters plus a NULL
    std::array<std::array<char, 81>, 4> authorCommentBuffers;
    TgaDateTime dateTimeStamp;
    std::array<std::byte, 41> jobId;
    TgaJobTime jobTime;
    std::array<std::byte, 41> softwareId;
    std::array<std::byte, 3> softwareVersion;
    TgaKeyColor keyColor;
    TgaRatio pixelAspectRatio;
    TgaRatio gammaValue;
    // Number of bytes from the beginning of the file to the color correction table if present
    uint32_t colorCorrectionOffset = 0;
    // Number of bytes from the beginning of the file to the postage stamp image if present
    uint32_t postageStampOffset = 0;
    // Number of bytes from the beginning of the file to the scan lines table if present
    uint32_t scanLineOffset = 0;
    // Specifies the alpha channel
    TgaAlphaType attributesType = TgaAlphaType::NoAlpha;
};

class TargaTexImpImporter final : public TextureImporter
{
public:
    enum class ImageOrigin
    {
        LowerLeft = 0b00,
        LowerRight = 0b01,
        UpperLeft = 0b10,
        UpperRight = 0b11
    };

    // Inherited via TextureImporter
    virtual FileFormat fileFormat() const final;

protected:
    virtual bool checkSignature(std::istream& stream) final;
    virtual void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) final;

private:
    [[nodiscard]] bool hasAlpha() const;
    [[nodiscard]] bool keepAlphaValue() const;

    [[nodiscard]] bool readBaseHeader(std::istream& stream);
    [[nodiscard]] void readExtensionHeader(std::istream& stream);

    [[nodiscard]] gpufmt::Format queryFormatForGrayScaleImage(ITextureAllocator& textureAllocator, TextureImportOptions options);
    [[nodiscard]] gpufmt::Format queryFormatFor555xImage(ITextureAllocator& textureAllocator, TextureImportOptions options);
    [[nodiscard]] gpufmt::Format queryFormatFor5551Image(ITextureAllocator& textureAllocator, TextureImportOptions options);
    [[nodiscard]] gpufmt::Format queryFormatForRgbImage(ITextureAllocator& textureAllocator, TextureImportOptions options);
    [[nodiscard]] gpufmt::Format queryFormatForRgbxImage(ITextureAllocator& textureAllocator, TextureImportOptions options);
    [[nodiscard]] gpufmt::Format queryFormatForRgbaImage(ITextureAllocator& textureAllocator, TextureImportOptions options);

    void readTrueColor(std::istream& stream, cputex::SurfaceSpan surface);
    void readTrueColorRLE(std::istream& stream, cputex::SurfaceSpan surface);
    void readGrayScale(std::istream& stream, cputex::SurfaceSpan surface);
    void readGrayScaleRLE(std::istream& stream, cputex::SurfaceSpan surface);
    void readColorMap(std::istream& stream, cputex::SurfaceSpan surface);
    void readColorMapRLE(std::istream& stream, cputex::SurfaceSpan surface);

    TgaHeader mHeader;
    ImageOrigin mImageOrigin;

    std::vector<std::byte> mImageId;
    std::vector<std::byte> mColorMapData;

    TgaFooter mFooter;
    TgaExtensionHeader mExtensionHeader;
    bool mFooterFound = false;
};
} // namespace teximp::targa

#endif // TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP