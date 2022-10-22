#pragma once

#include <teximp/teximp.h>

#ifdef TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP

namespace teximp::targa
{
class TargaTexImpImporter final : public TextureImporter
{
public:
    // Inherited via TextureImporter
    virtual FileFormat fileFormat() const final;

protected:
    virtual bool checkSignature(std::istream& stream) final;
    virtual void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) final;

private:
    struct TGAFooter
    {
        uint32_t extensionOffset;
        uint32_t developerAreaOffset;
        std::array<char, 16> signature;
        std::array<char, 2> end;
    };

    enum class ImageOrigin
    {
        LowerLeft = 0b00,
        LowerRight = 0b10,
        UpperLeft = 0b01,
        UpperRight = 0b11
    };

    bool readBaseHeader(std::istream& stream);

    void readTrueColor(std::istream& stream, std::span<std::byte> surfaceSpan);
    void readTrueColorRLE(std::istream& stream, std::span<std::byte> surfaceSpan);
    void readColorMap(std::istream& stream, std::span<std::byte> surfaceSpan);
    void readColorMapRLE(std::istream& stream, std::span<std::byte> surfaceSpan);
    void readGrayScale(std::istream& stream, std::span<std::byte> surfaceSpan);
    void readGrayScaleRLE(std::istream& stream, std::span<std::byte> surfaceSpan);

    // color map type
    //  0: no color map
    //  1: present
    //  2 - 127: reserved
    //  128 - 255: user data

    // image type
    //  0: no image data
    //  1: uncompressed color mapped image
    //  2: uncompresssed true color image
    //  3: uncompressed grayscale
    //  9: RLE color mapped
    //  10: RLE true color
    //  11: RLE grayscale

    uint8_t mIdLength = 0;
    uint8_t mColorMapType = 0;
    uint8_t mImageType = 0;
    // color map specification
    uint16_t mColorMapStartIndex = 0;
    uint16_t mColorMapCount = 0;
    uint8_t mColorMapEntrySize = 0;
    // image specification
    uint16_t mOriginX = 0;
    uint16_t mOriginY = 0;
    uint16_t mWidth = 0;
    uint16_t mHeight = 0;
    uint8_t mBitsPerPixel = 0;
    uint8_t mImageDescriptor = 0; // bits 3-0 give the alpha channel depth, bits 5-4 give direction

    ImageOrigin mImageOrigin;
    uint8_t mNumAlphaBits = 0;

    std::vector<uint8_t> mImageId;
    std::vector<uint8_t> mColorMapData;

    TGAFooter mFooter;

    bool mFooterFound = false;
};
} // namespace teximp::targa

#endif // TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP