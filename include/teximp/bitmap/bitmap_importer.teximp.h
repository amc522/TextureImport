#pragma once

#include <teximp/teximp.h>

#ifdef TEXIMP_ENABLE_BITMAP_BACKEND_TEXIMP

#include <glm/glm.hpp>

namespace teximp::bitmap
{
enum class BitmapCompression : uint32_t
{
    RGB = 0,
    RLE8,
    RLE4,
    Bitfields,
    Jpeg,
    Png,
    AlphaBitfields
};

enum class BitmapColorSpace : uint32_t
{
    CalibratedRGB = 0,
    sRGB = 'sRGB',
    WindowsColorSpace = 'Win ',
    ProfileLinked = 'LINK',
    ProfileEmbedded = 'MBED'
};

enum class BitmapIntent : uint32_t
{
    AbsColorimetric = 1,
    Business = 2,
    Graphics = 4,
    Images = 8
};

struct BitmapFileHeader
{
    uint16_t fileType;
    uint32_t fileSize;
    uint16_t reserved0;
    uint16_t reserved1;
    uint32_t bitmapOffset;
};

static_assert(sizeof(BitmapFileHeader) == 16, "");

struct BitmapHeaderV2
{
    uint32_t size;
    int16_t width;
    int16_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
};

struct BitmapRGBTriple
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
};

static_assert(sizeof(BitmapHeaderV2) == 12, "");
static_assert(sizeof(BitmapRGBTriple) == 3, "");

struct BitmapHeaderV3
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    BitmapCompression compression;
    uint32_t sizeOfBitmap;
    int32_t horizontalResolution;
    int32_t verticalResolution;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
};

struct BitmapRGBQuad
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t padding;
};

static_assert(sizeof(BitmapHeaderV3) == 40, "");
static_assert(sizeof(BitmapRGBQuad) == 4, "");

struct BitmapHeaderV3_52
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    BitmapCompression compression;
    uint32_t sizeOfBitmap;
    int32_t horizontalResolution;
    int32_t verticalResolution;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
};

static_assert(sizeof(BitmapHeaderV3_52) == 52, "");

struct BitmapHeaderV3_56
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    BitmapCompression compression;
    uint32_t sizeOfBitmap;
    int32_t horizontalResolution;
    int32_t verticalResolution;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t alphaMask;
};

static_assert(sizeof(BitmapHeaderV3_56) == 56, "");

struct BitmapHeaderOs2V2_16
{
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
};

static_assert(sizeof(BitmapHeaderOs2V2_16) == 16, "");

struct BitmapHeaderOs2V2
{
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    BitmapCompression compression;
    uint32_t sizeOfBitmap;
    uint32_t horizontalResolution;
    uint32_t verticalResolution;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
    uint16_t resUnit;
    uint16_t reserved;
    uint16_t orientation;
    uint16_t halftoning;
    uint32_t halftoneSize1;
    uint32_t halftoneSize2;
    uint32_t colorSpace;
    uint32_t appData;
};

static_assert(sizeof(BitmapHeaderOs2V2) == 64, "");

struct BitmapHeaderV4
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    BitmapCompression compression;
    uint32_t sizeOfBitmap;
    int32_t horizontalResolution;
    int32_t verticalResolution;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t alphaMask;
    BitmapColorSpace colorSpaceType;
    glm::u32vec3 redCoord;
    glm::u32vec3 greenCoord;
    glm::u32vec3 blueCoord;
    uint32_t gammaRed;
    uint32_t gammaGreen;
    uint32_t gammaBlue;
};

static_assert(sizeof(BitmapHeaderV4) == 108, "");

struct BitmapHeaderV5
{
    uint32_t size;
    int32_t width;
    int32_t height;
    uint16_t planes;
    uint16_t bitsPerPixel;
    BitmapCompression compression;
    uint32_t sizeOfBitmap;
    int32_t horizontalResolution;
    int32_t verticalResolution;
    uint32_t colorsUsed;
    uint32_t colorsImportant;
    uint32_t redMask;
    uint32_t greenMask;
    uint32_t blueMask;
    uint32_t alphaMask;
    BitmapColorSpace colorSpaceType;
    glm::u32vec3 redCoord;
    glm::u32vec3 greenCoord;
    glm::u32vec3 blueCoord;
    uint32_t gammaRed;
    uint32_t gammaGreen;
    uint32_t gammaBlue;
    BitmapIntent intent;
    uint32_t profileData;
    uint32_t profileSize;
    uint32_t reserved;
};

static_assert(sizeof(BitmapHeaderV5) == 124, "");

class BitmapTexImpImporter final : public TextureImporter
{
public:
    virtual FileFormat fileFormat() const final;

protected:
    virtual bool checkSignature(std::istream& stream) final;
    virtual void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) final;

private:
    enum class HeaderVersion
    {
        V2,
        V3,
        Os2V2,
        Os2V2_16,
        V4,
        V5
    };

    union Data24_32
    {
        uint8_t bytes24[3];
        uint32_t bytes32;
    };

    void loadV2(std::istream& stream, ITextureAllocator& textureAllocator);
    void loadV3(std::istream& stream, ITextureAllocator& textureAllocator);
    void loadV3_52(std::istream& stream, ITextureAllocator& textureAllocator);
    void loadV3_56(std::istream& stream, ITextureAllocator& textureAllocator);
    void loadV4(std::istream& stream, ITextureAllocator& textureAllocator);
    void loadV5(std::istream& stream, ITextureAllocator& textureAllocator);
    void loadOs2V2_16(std::istream& stream, ITextureAllocator& textureAllocator);
    void loadOs2V2(std::istream& stream, ITextureAllocator& textureAllocator);

    bool validateHeader();
    void calcDataSize(std::istream& stream);
    std::vector<BitmapRGBQuad> loadColorPalette(std::istream& stream, HeaderVersion headerVersion);
    void loadBitmap(std::istream& stream, ITextureAllocator& textureAllocator, HeaderVersion headerVersion);

    void read1BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);
    void read1BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);

    void read2BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);
    void read2BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);

    void read4BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);
    void read4BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);

    bool readRLE4Row(std::istream& stream, std::span<glm::u8vec3> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);
    bool readRLE4Row(std::istream& stream, std::span<glm::u8vec4> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);

    void read8BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);
    void read8BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);

    bool readRLE8Row(std::istream& stream, std::span<glm::u8vec3> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);
    bool readRLE8Row(std::istream& stream, std::span<glm::u8vec4> textureRow,
                     std::span<const BitmapRGBQuad> colorPalette);

    void read16BitRow_555(std::istream& stream, std::span<uint16_t> textureRow);
    void read16BitRow_565(std::istream& stream, std::span<uint16_t> textureRow);
    void read16BitRow_RGBMask(std::istream& stream, std::span<glm::u8vec3> textureRow);
    void read16BitRow_RGBMask(std::istream& stream, std::span<glm::u8vec4> textureRow);
    void read16BitRow_RGBAMask(std::istream& stream, std::span<glm::u8vec4> textureRow);

    void read24BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow);
    void read24BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow);
    void read24BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                      std::span<const BitmapRGBQuad> colorPalette);
    void read24BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                      std::span<const BitmapRGBQuad> colorPalette);

    void read32BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow);
    void read32BitRow_Mask(std::istream& stream, std::span<glm::u8vec4> textureRow);

    BitmapFileHeader mFileHeader;
    BitmapHeaderV5 mHeader;
    uint32_t mWidth;
    uint32_t mHeight;
    uint32_t mBytesPerRow;
    uint32_t mDataSize;
    glm::u32vec4 mMask;
    size_t mReadOffset = 0;
    uint32_t mRowsToSkip = 0;
    uint32_t mRowOffset = 0;
    std::vector<uint8_t> mPixelBuffer;
    TextureImportOptions mOptions;
};
} // namespace teximp::bitmap

#endif // TEXIMP_ENABLE_BITMAP_BACKEND_TEXIMP