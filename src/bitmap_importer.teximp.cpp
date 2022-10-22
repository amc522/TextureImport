#include <teximp/bitmap/bitmap_importer.teximp.h>

#ifdef TEXIMP_ENABLE_BITMAP_BACKEND_TEXIMP

#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/raw_data.hpp>
#include <gpufmt/utility.h>

#include <bitset>
#include <format>

namespace teximp::bitmap
{
template<class T>
class ColorConverter
{};

template<>
class ColorConverter<glm::u8vec3>
{
public:
    static glm::u8vec3 convert(std::span<const BitmapRGBQuad> colorPalette, size_t index)
    {
        if(!colorPalette.empty() && index < static_cast<size_t>(colorPalette.size()))
        {
            return convert(colorPalette[index]);
        }

        static BitmapRGBQuad blackAlpha = {0, 0, 0, 0};
        return convert(blackAlpha);
    }

    static glm::u8vec3 convert(BitmapRGBQuad color) { return glm::u8vec3(color.red, color.green, color.blue); }

    static glm::u8vec3 convert(uint16_t raw)
    {
        glm::u8vec3 color;
        color.b = static_cast<uint8_t>(raw & 0b0000'0000'0001'1111U);
        color.g = static_cast<uint8_t>((raw & 0b0000'0011'1110'0000U) >> 5);
        color.r = static_cast<uint8_t>((raw & 0b0111'1100'0000'0000U) >> 10);

        return color;
    }
};

template<>
class ColorConverter<glm::u8vec4>
{
public:
    static glm::u8vec4 convert(std::span<const BitmapRGBQuad> colorPalette, size_t index)
    {
        if(!colorPalette.empty() && index < static_cast<size_t>(colorPalette.size()))
        {
            return convert(colorPalette[index]);
        }

        static BitmapRGBQuad blackAlpha = {0, 0, 0, 0};
        return convert(blackAlpha);
    }

    static glm::u8vec4 convert(BitmapRGBQuad color) { return glm::u8vec4(color.red, color.green, color.blue, 255); }

    glm::u8vec4 convert(uint16_t raw)
    {
        glm::u8vec4 color;
        color.b = static_cast<uint8_t>(raw & 0b0000'0000'0001'1111U);
        color.g = static_cast<uint8_t>((raw & 0b0000'0011'1110'0000U) >> 5);
        color.r = static_cast<uint8_t>((raw & 0b0111'1100'0000'0000U) >> 10);
        color.a = 255;

        return color;
    }
};

constexpr float convertFixedPoint2_30(uint32_t raw)
{
    const float integerPart = static_cast<float>(raw >> 30);
    const float fractionalPart = (raw & 0x3fffffff) / 1073741823.0f;

    return integerPart + fractionalPart;
}

glm::vec3 convertFixedPoint2_30(glm::u32vec3 raw)
{
    return glm::vec3(convertFixedPoint2_30(raw.x), convertFixedPoint2_30(raw.y), convertFixedPoint2_30(raw.z));
}

constexpr float convertFixedPoint16_16(uint32_t raw)
{
    const float integerPart = static_cast<float>(raw >> 16);
    const float fractionalPart = (raw & 0x0000ffff) / 65535.0f;

    return integerPart + fractionalPart;
}

glm::vec3 convertFixedPoint16_16(glm::u32vec3 raw)
{
    return glm::vec3(convertFixedPoint16_16(raw.x), convertFixedPoint16_16(raw.y), convertFixedPoint16_16(raw.z));
}

uint8_t lowestOneBit(uint32_t bitfield)
{
    if(bitfield == 0) { return 0; }

    uint8_t count = 0;
    while((bitfield & 1) == 0)
    {
        ++count;
        bitfield >>= 1;
    }

    return count;
}

uint8_t highestOneBit(uint32_t bitfield)
{
    if(bitfield == 0) { return 0; }

    uint8_t count = 32;
    while((bitfield & 0x80000000) == 0)
    {
        --count;
        bitfield <<= 1;
    }

    return count;
}

FileFormat BitmapTexImpImporter::fileFormat() const
{
    return FileFormat::Bitmap;
}

bool BitmapTexImpImporter::checkSignature(std::istream& stream)
{
    const std::array<char, 2> expectedSignature = {'B', 'M'};
    std::array<char, 2> actualSignature;

    stream.read(actualSignature.data(), 2);

    return expectedSignature == actualSignature;
}

void BitmapTexImpImporter::load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options)
{
    mOptions = options;

    stream.seekg(0);

    stream.read(reinterpret_cast<char*>(&mFileHeader.fileType), sizeof(mFileHeader.fileType));
    stream.read(reinterpret_cast<char*>(&mFileHeader.fileSize), sizeof(mFileHeader.fileSize));
    stream.read(reinterpret_cast<char*>(&mFileHeader.reserved0), sizeof(mFileHeader.reserved0));
    stream.read(reinterpret_cast<char*>(&mFileHeader.reserved1), sizeof(mFileHeader.reserved1));
    stream.read(reinterpret_cast<char*>(&mFileHeader.bitmapOffset), sizeof(mFileHeader.bitmapOffset));

    uint32_t headerSize;
    stream.read(reinterpret_cast<char*>(&headerSize), sizeof(uint32_t));
    stream.seekg(-(std::streampos)sizeof(headerSize), std::ios_base::cur);

    memset(&mHeader, 0, sizeof(mHeader));

    switch(headerSize)
    {
    case sizeof(BitmapHeaderV2): loadV2(stream, textureAllocator); break;
    case sizeof(BitmapHeaderV3): loadV3(stream, textureAllocator); break;
    case sizeof(BitmapHeaderV3_52): loadV3_52(stream, textureAllocator); break;
    case sizeof(BitmapHeaderV3_56): loadV3_56(stream, textureAllocator); break;
    case sizeof(BitmapHeaderV4): loadV4(stream, textureAllocator); break;
    case sizeof(BitmapHeaderV5): loadV5(stream, textureAllocator); break;
    case sizeof(BitmapHeaderOs2V2_16): loadOs2V2_16(stream, textureAllocator); break;
    case sizeof(BitmapHeaderOs2V2): loadOs2V2(stream, textureAllocator); break;
    default:
        setError(TextureImportError::CouldNotReadHeader, std::format("Unrecognized header size '{}'.", headerSize));
        return;
    }
}

void BitmapTexImpImporter::loadV2(std::istream& stream, ITextureAllocator& textureAllocator)
{
    BitmapHeaderV2 header;
    stream.read(reinterpret_cast<char*>(&header), sizeof(header));

    mHeader.size = header.size;
    mHeader.width = header.width;
    mHeader.height = header.height;
    mHeader.planes = header.planes;
    mHeader.bitsPerPixel = header.bitsPerPixel;
    mHeader.compression = BitmapCompression::RGB;
    mHeader.sizeOfBitmap = 0;
    mHeader.horizontalResolution = 0;
    mHeader.verticalResolution = 0;
    mHeader.colorsUsed = 0;
    mHeader.colorsImportant = 0;
    mHeader.redMask = 0;
    mHeader.greenMask = 0;
    mHeader.blueMask = 0;
    mHeader.alphaMask = 0;
    mHeader.colorSpaceType = BitmapColorSpace::CalibratedRGB;
    mHeader.redCoord;
    mHeader.greenCoord;
    mHeader.blueCoord;
    mHeader.gammaRed = 0;
    mHeader.gammaGreen = 0;
    mHeader.gammaBlue = 0;
    mHeader.intent = BitmapIntent::AbsColorimetric;
    mHeader.profileData = 0;
    mHeader.profileSize = 0;
    mHeader.reserved = 0;

    loadBitmap(stream, textureAllocator, HeaderVersion::V2);
}

void BitmapTexImpImporter::loadV3(std::istream& stream, ITextureAllocator& textureAllocator)
{
    stream.read(reinterpret_cast<char*>(&mHeader), sizeof(BitmapHeaderV3));

    if(mHeader.compression == BitmapCompression::Bitfields || mHeader.compression == BitmapCompression::AlphaBitfields)
    {
        stream.read(reinterpret_cast<char*>(&mHeader.redMask), sizeof(mHeader.redMask) * 4);
    }
    else
    {
        mHeader.redMask = 0;
        mHeader.greenMask = 0;
        mHeader.blueMask = 0;
        mHeader.alphaMask = 0;
    }

    loadBitmap(stream, textureAllocator, HeaderVersion::V3);
}

void BitmapTexImpImporter::loadV3_52(std::istream& stream, ITextureAllocator& textureAllocator)
{
    stream.read(reinterpret_cast<char*>(&mHeader), sizeof(BitmapHeaderV3_52));
    loadBitmap(stream, textureAllocator, HeaderVersion::V3);
}

void BitmapTexImpImporter::loadV3_56(std::istream& stream, ITextureAllocator& textureAllocator)
{
    stream.read(reinterpret_cast<char*>(&mHeader), sizeof(BitmapHeaderV3_56));
    loadBitmap(stream, textureAllocator, HeaderVersion::V3);
}

void BitmapTexImpImporter::loadV4(std::istream& stream, ITextureAllocator& textureAllocator)
{
    stream.read(reinterpret_cast<char*>(&mHeader), sizeof(BitmapHeaderV4));

    mHeader.intent = BitmapIntent::AbsColorimetric;
    mHeader.profileData = 0;
    mHeader.profileSize = 0;
    mHeader.reserved = 0;

    loadBitmap(stream, textureAllocator, HeaderVersion::V4);
}

void BitmapTexImpImporter::loadV5(std::istream& stream, ITextureAllocator& textureAllocator)
{
    stream.read(reinterpret_cast<char*>(&mHeader), sizeof(BitmapHeaderV5));

    loadBitmap(stream, textureAllocator, HeaderVersion::V5);
}

void BitmapTexImpImporter::loadOs2V2_16(std::istream& stream, ITextureAllocator& textureAllocator)
{
    stream.read(reinterpret_cast<char*>(&mHeader), sizeof(BitmapHeaderOs2V2_16));

    BitmapHeaderOs2V2_16 header;
    memcpy(&header, &mHeader, sizeof(header));

    loadBitmap(stream, textureAllocator, HeaderVersion::Os2V2_16);
}

void BitmapTexImpImporter::loadOs2V2(std::istream& stream, ITextureAllocator& textureAllocator)
{
    stream.read(reinterpret_cast<char*>(&mHeader), sizeof(BitmapHeaderOs2V2));

    BitmapHeaderOs2V2 header;
    memcpy(&header, &mHeader, sizeof(BitmapHeaderOs2V2));
    loadBitmap(stream, textureAllocator, HeaderVersion::Os2V2);
}

bool BitmapTexImpImporter::validateHeader()
{
    // if(mHeader.planes != 1) {
    //     setError(TextureImportError::Unknown, "Number of color planes greater than 1");
    //     return false;
    // }

    if(mWidth > kMaxTextureWidth)
    {
        setError(TextureImportError::DimensionsTooLarge,
                 std::format("Image width of {} pixels is greater than the max supported width of {} pixels.", mWidth,
                             kMaxTextureWidth));
        return false;
    }

    if(mHeight > kMaxTextureHeight)
    {
        setError(TextureImportError::DimensionsTooLarge,
                 std::format("Image height of {} pixels is greater than the max supported height of {} pixels.",
                             mHeight, kMaxTextureHeight));
        return false;
    }

    if(mHeader.bitsPerPixel != 1 && mHeader.bitsPerPixel != 2 && mHeader.bitsPerPixel != 4 &&
       mHeader.bitsPerPixel != 8 && mHeader.bitsPerPixel != 16 && mHeader.bitsPerPixel != 24 &&
       mHeader.bitsPerPixel != 32)
    {
        setError(TextureImportError::Unknown, std::format("Invalid bits per pixel value '{}'", mHeader.bitsPerPixel));
        return false;
    }

    if(mHeader.compression > BitmapCompression::AlphaBitfields)
    {
        setError(TextureImportError::Unknown, "Invalid compression type");
        return false;
    }

    return true;
}

void BitmapTexImpImporter::calcDataSize(std::istream& stream)
{
    if(mHeader.compression == BitmapCompression::RGB || mHeader.compression == BitmapCompression::Bitfields ||
       mHeader.compression == BitmapCompression::AlphaBitfields)
    {
        mDataSize = (mWidth * mHeader.bitsPerPixel + 7) / 8;
        mDataSize += 3 - ((mDataSize + 3) % 4);
        mDataSize *= mHeight;
    }
    else
    {
        uint32_t dataStart = mFileHeader.bitmapOffset;

        if(dataStart == 0) { dataStart = (uint32_t)stream.tellg(); }

        stream.seekg(0, std::ios_base::end);
        mDataSize =
            glm::min(static_cast<uint32_t>(mHeader.sizeOfBitmap), static_cast<uint32_t>(stream.tellg()) - dataStart);
        stream.seekg(dataStart);
    }
}

std::vector<BitmapRGBQuad> BitmapTexImpImporter::loadColorPalette(std::istream& stream, HeaderVersion headerVersion)
{
    uint32_t colorsUsed = mHeader.colorsUsed;
    if(colorsUsed == 0 && mHeader.bitsPerPixel < 16) { colorsUsed = 1 << mHeader.bitsPerPixel; }

    std::vector<BitmapRGBQuad> colorPalette;
    if(colorsUsed > 0)
    {
        colorPalette.resize(glm::min(colorsUsed, 256U));

        if(headerVersion >= HeaderVersion::V3)
        {
            stream.read(reinterpret_cast<char*>(colorPalette.data()), colorPalette.size() * sizeof(BitmapRGBQuad));

            if(colorsUsed > 256) { stream.seekg((colorsUsed - 256) * sizeof(BitmapRGBQuad), std::ios_base::cur); }
        }
        else
        {
            std::vector<BitmapRGBTriple> colorPaletteV2(colorsUsed);
            stream.read(reinterpret_cast<char*>(colorPaletteV2.data()),
                        colorPaletteV2.size() * sizeof(BitmapRGBTriple));

            std::transform(std::begin(colorPaletteV2), std::end(colorPaletteV2), std::begin(colorPalette),
                           [](const BitmapRGBTriple& paletteV2)
                           {
                               BitmapRGBQuad paletteV3;
                               paletteV3.blue = paletteV2.blue;
                               paletteV3.green = paletteV2.green;
                               paletteV3.red = paletteV2.red;
                               return paletteV3;
                           });

            if(colorsUsed > 256) { stream.seekg((colorsUsed - 256) * sizeof(BitmapRGBTriple), std::ios_base::cur); }
        }
    }

    return colorPalette;
}

void BitmapTexImpImporter::loadBitmap(std::istream& stream, ITextureAllocator& textureAllocator,
                                      HeaderVersion headerVersion)
{
    mWidth = std::abs(mHeader.width);
    mHeight = std::abs(mHeader.height);

    if(!validateHeader()) { return; }

    std::vector<BitmapRGBQuad> colorPalette = loadColorPalette(stream, headerVersion);

    calcDataSize(stream);

    bool useBitfields = false;
    if(mHeader.compression == BitmapCompression::Bitfields || mHeader.compression == BitmapCompression::AlphaBitfields)
    {
        useBitfields = true;
        if(mHeader.redMask == 0 && mHeader.greenMask == 0 && mHeader.blueMask == 0 && mHeader.alphaMask == 0)
        {
            switch(mHeader.bitsPerPixel)
            {
            case 16:
                mMask.a = 0;
                mMask.r = 0b11111'000000'00000'0000'0000'0000'0000;
                mMask.g = 0b00000'111111'00000'0000'0000'0000'0000;
                mMask.b = 0b00000'000000'11111'0000'0000'0000'0000;
                break;
            case 24:
                mMask.a = 0;
                mMask.r = 0b1111'1111'0000'0000'0000'0000'0000'0000;
                mMask.g = 0b0000'0000'1111'1111'0000'0000'0000'0000;
                mMask.b = 0b0000'0000'0000'0000'1111'1111'0000'0000;
                break;
            case 32:
                mMask.a = 0b1111'1111'0000'0000'0000'0000'0000'0000;
                mMask.r = 0b0000'0000'1111'1111'0000'0000'0000'0000;
                mMask.g = 0b0000'0000'0000'0000'1111'1111'0000'0000;
                mMask.b = 0b0000'0000'0000'0000'0000'0000'1111'1111;
                break;
            }
        }
        else
        {
            mMask.r = mHeader.redMask;
            mMask.g = mHeader.greenMask;
            mMask.b = mHeader.blueMask;
            mMask.a = mHeader.alphaMask;
        }
    }

    if(mHeader.compression == BitmapCompression::RGB || useBitfields) { mBytesPerRow = mDataSize / mHeight; }
    else if(mHeader.compression == BitmapCompression::RLE4 || mHeader.compression == BitmapCompression::RLE8)
    {
        // multiplying by three because thats the worst case scenario for a really bad RLE compression
        mBytesPerRow = (mWidth * mHeader.bitsPerPixel + 7) / 8 * 3;
    }

    glm::vec3 redEndpoint;
    glm::vec3 greenEndpoint;
    glm::vec3 blueEndpoint;
    glm::vec3 gamma;

    if(headerVersion >= HeaderVersion::V4)
    {
        redEndpoint = convertFixedPoint2_30(mHeader.redCoord);
        greenEndpoint = convertFixedPoint2_30(mHeader.greenCoord);
        blueEndpoint = convertFixedPoint2_30(mHeader.blueCoord);
        gamma.r = convertFixedPoint16_16(mHeader.gammaRed);
        gamma.g = convertFixedPoint16_16(mHeader.gammaGreen);
        gamma.b = convertFixedPoint16_16(mHeader.gammaBlue);
    }

    gpufmt::Format format = gpufmt::Format::R8G8B8_UNORM;

    if(mOptions.padRgbWithAlpha) { format = gpufmt::Format::R8G8B8A8_UNORM; }

    static constexpr gpufmt::Format compressed5551Format = gpufmt::Format::R5G5B5A1_UNORM_PACK16;

    if(mHeader.bitsPerPixel < 16)
    {
        if(mOptions.padRgbWithAlpha) { format = gpufmt::Format::R8G8B8A8_UNORM; }
        else { format = gpufmt::Format::R8G8B8_UNORM; }
    }
    else if(mHeader.bitsPerPixel == 16)
    {
        if(mHeader.compression == BitmapCompression::RGB) { format = compressed5551Format; }
        else if(mHeader.compression == BitmapCompression::Bitfields)
        {
            if(glm::bitCount(mMask.r) == 5 && glm::bitCount(mMask.g) == 6 && glm::bitCount(mMask.b))
            {
                format = gpufmt::Format::R5G6B5_UNORM_PACK16;
            }
            else if(glm::bitCount(mMask.r) == 5 && glm::bitCount(mMask.g) == 5 && glm::bitCount(mMask.b))
            {
                format = compressed5551Format;
            }
            else if((mMask.a & 0xffff) > 0) { format = gpufmt::Format::R8G8B8A8_UNORM; }
            else { format = gpufmt::Format::R8G8B8_UNORM; }
        }
    }
    else if(mHeader.bitsPerPixel == 24)
    {
        if(mOptions.padRgbWithAlpha) { format = gpufmt::Format::R8G8B8A8_UNORM; }
        else { format = gpufmt::Format::R8G8B8_UNORM; }
    }
    else if(mHeader.bitsPerPixel == 32) { format = gpufmt::Format::R8G8B8A8_UNORM; }

    if(mHeader.colorSpaceType == BitmapColorSpace::sRGB) { format = gpufmt::sRGBFormat(format).value_or(format); }

    if(mFileHeader.bitmapOffset != 0) { stream.seekg(mFileHeader.bitmapOffset); }

    textureAllocator.preAllocation(1);

    cputex::TextureParams textureParams{
        .format = format,
        .dimension = cputex::TextureDimension::Texture2D,
        .extent = cputex::Extent{mWidth, mHeight, 1},
        .arraySize = 1,
        .faces = 1,
        .mips = 1
    };

    const bool allocationSuccess = textureAllocator.allocateTexture(textureParams, 0);

    if(!allocationSuccess)
    {
        setTextureAllocationError(textureParams);
        return;
    }

    textureAllocator.postAllocation();

    std::span<std::byte> textureData =
        textureAllocator.accessTextureData(0, MipSurfaceKey{.arraySlice = 0, .face = 0, .mip = 0});

    std::span<glm::u8vec3> textureDataRGB8;
    std::span<glm::u8vec4> textureDataRGBA8;
    std::span<uint16_t> textureDataRGB16;
    std::span<uint16_t> textureDataRGBA16;

    int32_t y;
    int32_t yEnd;
    int32_t direction;
    if(mHeader.height > 0)
    {
        y = mHeight - 1;
        yEnd = -1;
        direction = -1;
    }
    else
    {
        y = 0;
        yEnd = mHeight;
        direction = 1;
    }

    mPixelBuffer.resize(mBytesPerRow);

    switch(mHeader.bitsPerPixel)
    {
    case 1:
        if(!mOptions.padRgbWithAlpha)
        {
            textureDataRGB8 = castWritableBytes<glm::u8vec3>(textureData);

            for(; y != yEnd; y += direction)
            {
                read1BitRow(stream, textureDataRGB8.subspan(y * mWidth, mWidth), colorPalette);
            }
        }
        else
        {
            textureDataRGBA8 = castWritableBytes<glm::u8vec4>(textureData);

            for(; y != yEnd; y += direction)
            {
                read1BitRow(stream, textureDataRGBA8.subspan(y * mWidth, mWidth), colorPalette);
            }
        }
        break;
    case 2:
        if(!mOptions.padRgbWithAlpha)
        {
            textureDataRGB8 = castWritableBytes<glm::u8vec3>(textureData);

            for(; y != yEnd; y += direction)
            {
                read2BitRow(stream, textureDataRGB8.subspan(y * mWidth, mWidth), colorPalette);
            }
        }
        else
        {
            textureDataRGBA8 = castWritableBytes<glm::u8vec4>(textureData);

            for(; y != yEnd; y += direction)
            {
                read2BitRow(stream, textureDataRGBA8.subspan(y * mWidth, mWidth), colorPalette);
            }
        }
        break;
    case 4:
        if(!mOptions.padRgbWithAlpha)
        {
            textureDataRGB8 = castWritableBytes<glm::u8vec3>(textureData);

            if(mHeader.compression == BitmapCompression::RGB)
            {
                for(; y != yEnd; y += direction)
                {
                    read4BitRow(stream, textureDataRGB8.subspan(y * mWidth, mWidth), colorPalette);
                }
            }
            else
            {
                for(; y != yEnd; y += direction * glm::max(mRowsToSkip, 1U))
                {
                    if(!readRLE4Row(stream, textureDataRGB8.subspan(y * mWidth + mRowOffset, mWidth), colorPalette))
                    {
                        return;
                    }
                }
            }
        }
        else
        {
            textureDataRGBA8 = castWritableBytes<glm::u8vec4>(textureData);

            if(mHeader.compression == BitmapCompression::RGB)
            {
                for(; y != yEnd; y += direction)
                {
                    read4BitRow(stream, textureDataRGBA8.subspan(y * mWidth, mWidth), colorPalette);
                }
            }
            else
            {
                for(; y != yEnd; y += direction * glm::max(mRowsToSkip, 1U))
                {
                    if(!readRLE4Row(stream, textureDataRGBA8.subspan(y * mWidth + mRowOffset, mWidth), colorPalette))
                    {
                        return;
                    }
                }
            }
        }
        break;
    case 8:
        if(!mOptions.padRgbWithAlpha)
        {
            textureDataRGB8 = castWritableBytes<glm::u8vec3>(textureData);

            if(mHeader.compression == BitmapCompression::RGB)
            {
                for(; y != yEnd; y += direction)
                {
                    read8BitRow(stream, textureDataRGB8.subspan(y * mWidth, mWidth), colorPalette);
                }
            }
            else
            {
                for(; y != yEnd; y += direction * glm::max(mRowsToSkip, 1U))
                {
                    if(!readRLE8Row(stream, textureDataRGB8.subspan(y * mWidth + mRowOffset, mWidth), colorPalette))
                    {
                        return;
                    }
                }
            }
        }
        else
        {
            textureDataRGBA8 = castWritableBytes<glm::u8vec4>(textureData);

            if(mHeader.compression == BitmapCompression::RGB)
            {
                for(; y != yEnd; y += direction)
                {
                    read8BitRow(stream, textureDataRGBA8.subspan(y * mWidth, mWidth), colorPalette);
                }
            }
            else
            {
                for(; y != yEnd; y += direction * glm::max(mRowsToSkip, 1U))
                {
                    if(!readRLE8Row(stream, textureDataRGBA8.subspan(y * mWidth + mRowOffset, mWidth), colorPalette))
                    {
                        return;
                    }
                }
            }
        }
        break;
    case 16:
        if(mHeader.compression == BitmapCompression::RGB || useBitfields && format == compressed5551Format)
        {
            textureDataRGB16 = castWritableBytes<uint16_t>(textureData);

            for(; y != yEnd; y += direction)
            {
                read16BitRow_555(stream, textureDataRGB16.subspan(y * mWidth, mWidth));
            }
        }
        else if(useBitfields && format == gpufmt::Format::R5G6B5_UNORM_PACK16)
        {
            textureDataRGB16 = castWritableBytes<uint16_t>(textureData);

            for(; y != yEnd; y += direction)
            {
                read16BitRow_565(stream, textureDataRGB16.subspan(y * mWidth, mWidth));
            }
        }
        else if(useBitfields && (mMask.a & 0xffff) > 0)
        {
            textureDataRGBA8 = castWritableBytes<glm::u8vec4>(textureData);

            for(; y != yEnd; y += direction)
            {
                read16BitRow_RGBAMask(stream, textureDataRGBA8.subspan(y * mWidth, mWidth));
            }
        }
        else if(useBitfields && !mOptions.padRgbWithAlpha)
        {
            textureDataRGB8 = castWritableBytes<glm::u8vec3>(textureData);

            for(; y != yEnd; y += direction)
            {
                read16BitRow_RGBMask(stream, textureDataRGB8.subspan(y * mWidth, mWidth));
            }
        }
        else if(useBitfields && mOptions.padRgbWithAlpha)
        {
            textureDataRGBA8 = castWritableBytes<glm::u8vec4>(textureData);

            for(; y != yEnd; y += direction)
            {
                read16BitRow_RGBMask(stream, textureDataRGBA8.subspan(y * mWidth, mWidth));
            }
        }
        break;
    case 24:

        if(!mOptions.padRgbWithAlpha)
        {
            textureDataRGB8 = castWritableBytes<glm::u8vec3>(textureData);

            if(mHeader.compression == BitmapCompression::RGB && colorPalette.empty())
            {
                for(; y != yEnd; y += direction)
                {
                    read24BitRow(stream, textureDataRGB8.subspan(y * mWidth, mWidth));
                }
            }
            else if(mHeader.compression == BitmapCompression::RGB && !colorPalette.empty())
            {
                for(; y != yEnd; y += direction)
                {
                    read24BitRow(stream, textureDataRGB8.subspan(y * mWidth, mWidth), colorPalette);
                }
            }
        }
        else
        {
            textureDataRGBA8 = castWritableBytes<glm::u8vec4>(textureData);

            if(mHeader.compression == BitmapCompression::RGB && colorPalette.empty())
            {
                for(; y != yEnd; y += direction)
                {
                    read24BitRow(stream, textureDataRGBA8.subspan(y * mWidth, mWidth));
                }
            }
            else if(mHeader.compression == BitmapCompression::RGB && !colorPalette.empty())
            {
                for(; y != yEnd; y += direction)
                {
                    read24BitRow(stream, textureDataRGBA8.subspan(y * mWidth, mWidth), colorPalette);
                }
            }
        }
        break;
    case 32:
        textureDataRGBA8 = castWritableBytes<glm::u8vec4>(textureData);

        if(mHeader.compression == BitmapCompression::RGB)
        {
            for(; y != yEnd; y += direction)
            {
                read32BitRow(stream, textureDataRGBA8.subspan(y * mWidth, mWidth));
            }
        }
        else if(useBitfields)
        {
            for(; y != yEnd; y += direction)
            {
                read32BitRow_Mask(stream, textureDataRGBA8.subspan(y * mWidth, mWidth));
            }
        }
        break;
    default: break;
    }
}

BitmapRGBQuad getColor(std::span<const BitmapRGBQuad> colorPalette, size_t index)
{
    if(!colorPalette.empty() && index < static_cast<size_t>(colorPalette.size())) { return colorPalette[index]; }

    static BitmapRGBQuad blackAlpha = {0, 0, 0, 0};
    return blackAlpha;
}

template<class T>
void read1BitRowT(std::istream& stream, std::span<T> textureRow, std::span<const BitmapRGBQuad> colorPalette,
                  std::vector<glm::byte>& pixelBuffer, uint32_t bytesPerRow, uint32_t textureWidth)
{
    const size_t dataBytesPerRow = (textureWidth + 7) / 8;
    const size_t lastBit = dataBytesPerRow * 8 - textureWidth;
    size_t texelIndex = 0;

    stream.read(reinterpret_cast<char*>(pixelBuffer.data()), bytesPerRow);

    for(size_t x = 0; x < dataBytesPerRow - 1; ++x)
    {
        std::bitset<8> bits(pixelBuffer[x]);

        for(int8_t bit = 7; bit >= 0; --bit)
        {
            uint8_t paletteIndex = bits[bit];
            textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);
        }
    }

    std::bitset<8> bits(pixelBuffer[dataBytesPerRow - 1]);
    for(int8_t bit = 7; bit >= lastBit; --bit)
    {
        uint8_t paletteIndex = bits[bit];
        textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);
    }
}

void BitmapTexImpImporter::read1BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    read1BitRowT(stream, textureRow, colorPalette, mPixelBuffer, mBytesPerRow, mWidth);
}

void BitmapTexImpImporter::read1BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    read1BitRowT(stream, textureRow, colorPalette, mPixelBuffer, mBytesPerRow, mWidth);
}

template<class T>
void read2BitRowT(std::istream& stream, std::span<T> textureRow, std::span<const BitmapRGBQuad> colorPalette,
                  std::vector<glm::byte>& pixelBuffer, uint32_t bytesPerRow, uint32_t textureWidth)
{
    const size_t dataBytesPerRow = (textureWidth + 3) / 4;
    size_t remainderCount = textureWidth % 4;
    size_t texelIndex = 0;

    stream.read(reinterpret_cast<char*>(pixelBuffer.data()), bytesPerRow);

    union ByteData
    {
        uint8_t raw;
        struct
        {
            uint8_t bit01 :2;
            uint8_t bit23 :2;
            uint8_t bit45 :2;
            uint8_t bit67 :2;
        } data;
    };

    for(size_t x = 0; x < textureWidth / 4; ++x)
    {
        ByteData currByte;
        currByte.raw = pixelBuffer[x];

        // 67
        uint8_t paletteIndex = currByte.data.bit67;
        textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);

        // 45
        paletteIndex = currByte.data.bit45;
        textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);

        // 23
        paletteIndex = currByte.data.bit23;
        textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);

        // 01
        paletteIndex = currByte.data.bit01;
        textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);
    }

    for(uint8_t mask = 0b11000000, shift = 6; mask > 0 && remainderCount > 0; mask >>= 2, shift -= 2, --remainderCount)
    {
        uint8_t data = pixelBuffer[dataBytesPerRow - 1];
        uint8_t paletteIndex = (data & mask) >> shift;
        textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);
    }
}

void BitmapTexImpImporter::read2BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    read2BitRowT(stream, textureRow, colorPalette, mPixelBuffer, mBytesPerRow, mWidth);
}

void BitmapTexImpImporter::read2BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    read2BitRowT(stream, textureRow, colorPalette, mPixelBuffer, mBytesPerRow, mWidth);
}

template<class T>
void read4BitRowT(std::istream& stream, std::span<T> textureRow, std::span<const BitmapRGBQuad> colorPalette,
                  std::vector<glm::byte>& pixelBuffer, uint32_t bytesPerRow, const BitmapHeaderV5& header)
{
    const size_t dataBytesPerRow = (header.width + 1) / 2;
    const bool processExtra = (dataBytesPerRow * 2 - header.width) == 1;
    size_t texelIndex = 0;

    stream.read(reinterpret_cast<char*>(pixelBuffer.data()), bytesPerRow);

    union ByteData
    {
        uint8_t raw;
        struct
        {
            uint8_t low  :4;
            uint8_t high :4;
        } data;
    };

    const size_t end = (processExtra) ? dataBytesPerRow - 1 : dataBytesPerRow;
    for(size_t x = 0; x < end; ++x)
    {
        ByteData currByte;
        currByte.raw = pixelBuffer[x];

        // high bits
        uint8_t paletteIndex = currByte.data.high;
        textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);

        // low bits
        paletteIndex = currByte.data.low;
        textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);
    }

    if(processExtra)
    {
        ByteData currByte;
        currByte.raw = pixelBuffer[dataBytesPerRow - 1];

        // high bits
        uint8_t paletteIndex = currByte.data.high;
        textureRow[texelIndex++] = ColorConverter<T>::convert(colorPalette, paletteIndex);
    }
}

void BitmapTexImpImporter::read4BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    read4BitRowT(stream, textureRow, colorPalette, mPixelBuffer, mBytesPerRow, mHeader);
}

void BitmapTexImpImporter::read4BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    read4BitRowT(stream, textureRow, colorPalette, mPixelBuffer, mBytesPerRow, mHeader);
}

template<class T>
std::tuple<bool, TextureImportError, const char*>
readRLE4RowT(std::istream& stream, std::span<T> textureRow, std::span<const BitmapRGBQuad> colorPalette,
             std::vector<glm::byte>& pixelBuffer, uint32_t& rowOffset, uint32_t& rowsToSkip, size_t& readOffset,
             uint32_t textureWidth)
{
    union ByteData
    {
        uint8_t raw;
        struct
        {
            uint8_t low  :4;
            uint8_t high :4;
        } data;
    };

    rowOffset = 0;
    rowsToSkip = 0;

    stream.read(reinterpret_cast<char*>(pixelBuffer.data()) + readOffset, pixelBuffer.size() - readOffset);
    size_t bytesRead = stream.gcount();
    size_t bufferSize = readOffset + bytesRead;
    std::span<uint8_t> compressedData(pixelBuffer.data(), bufferSize);

    while(!compressedData.empty())
    {
        if(compressedData[0] == 0 && compressedData[1] == 0)
        {
            // end of scan line;
            compressedData = compressedData.subspan(2);

            if(!compressedData.empty())
            {
                std::rotate(std::begin(pixelBuffer), std::begin(pixelBuffer) + (bufferSize - compressedData.size()),
                            pixelBuffer.end());
                readOffset = compressedData.size();
            }
            else { readOffset = 0; }

            return std::make_tuple(true, TextureImportError::None, nullptr);
        }
        else if(compressedData[0] == 0 && compressedData[1] == 1)
        {
            // end of data
            return std::make_tuple(true, TextureImportError::None, nullptr);
        }
        else if(compressedData[0] == 0 && compressedData[1] == 2)
        {
            // move cursor
            uint32_t rowOffsetAbsolute = (textureWidth - static_cast<uint32_t>(textureRow.size()) + compressedData[2]);
            if(compressedData[3] > 0 || rowOffsetAbsolute >= textureWidth)
            {
                rowsToSkip = compressedData[3];

                if(rowOffsetAbsolute < textureWidth) { rowOffset = compressedData[2]; }
                else
                {
                    rowOffset = rowOffsetAbsolute % textureWidth;
                    rowsToSkip += rowOffsetAbsolute / textureWidth;
                }

                compressedData = compressedData.subspan(2);

                if(!compressedData.empty())
                {
                    std::rotate(std::begin(pixelBuffer), std::begin(pixelBuffer) + (bufferSize - compressedData.size()),
                                pixelBuffer.end());
                    readOffset = compressedData.size();
                }
                else { readOffset = 0; }

                return std::make_tuple(true, TextureImportError::None, nullptr);
            }

            textureRow = textureRow.subspan(compressedData[2]);
            compressedData = compressedData.subspan(4);
        }
        else if(compressedData[0] == 0)
        {
            // unencoded run
            size_t count = compressedData[1];
            compressedData = compressedData.subspan(2);

            const size_t textureWriteCount = glm::min(count, static_cast<size_t>(textureRow.size()));
            for(size_t i = 0; i < textureWriteCount / 2; ++i)
            {
                ByteData byteData;
                byteData.raw = compressedData[i];
                textureRow[i * 2] = ColorConverter<T>::convert(colorPalette, byteData.data.high);
                textureRow[i * 2 + 1] = ColorConverter<T>::convert(colorPalette, byteData.data.low);
            }

            if(textureWriteCount % 2 == 1)
            {
                ByteData byteData;
                byteData.raw = compressedData[count / 2];
                textureRow[count - 1] = ColorConverter<T>::convert(colorPalette, byteData.data.high);
            }

            if(count > textureWriteCount)
            {
                return std::make_tuple(false, TextureImportError::InvalidDataInImage,
                                       "RLE4 encoded stream ran beyond the end of a row.");
            }

            textureRow = textureRow.subspan(count);
            size_t unencodedBytes = (count + 1) / 2;
            size_t totalBytes = unencodedBytes + 2;
            totalBytes = (totalBytes + 1) / 2 * 2 - 2;
            compressedData = compressedData.subspan(totalBytes);
        }
        else
        {
            // repeated color
            size_t count = compressedData[0];
            ByteData byteData;
            byteData.raw = compressedData[1];
            std::array<T, 2> colors = {ColorConverter<T>::convert(colorPalette, byteData.data.high),
                                       ColorConverter<T>::convert(colorPalette, byteData.data.low)};

            const size_t textureWriteCount = glm::min(count, static_cast<size_t>(textureRow.size()));
            for(size_t i = 0; i < textureWriteCount; ++i)
            {
                textureRow[i] = colors[i % 2];
            }

            if(count > textureWriteCount)
            {
                return std::make_tuple(false, TextureImportError::InvalidDataInImage,
                                       "RLE4 encoded stream ran beyond the end of a row.");
            }

            textureRow = textureRow.subspan(textureWriteCount);
            compressedData = compressedData.subspan(2);
        }
    }

    return std::make_tuple(true, TextureImportError::None, nullptr);
}

bool BitmapTexImpImporter::readRLE4Row(std::istream& stream, std::span<glm::u8vec3> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    auto returnVal =
        readRLE4RowT(stream, textureRow, colorPalette, mPixelBuffer, mRowOffset, mRowsToSkip, mReadOffset, mWidth);

    if(std::get<0>(returnVal)) { return true; }
    else
    {
        setError(std::get<1>(returnVal), std::get<2>(returnVal));
        return false;
    }
}

bool BitmapTexImpImporter::readRLE4Row(std::istream& stream, std::span<glm::u8vec4> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    auto returnVal =
        readRLE4RowT(stream, textureRow, colorPalette, mPixelBuffer, mRowOffset, mRowsToSkip, mReadOffset, mWidth);

    if(std::get<0>(returnVal)) { return true; }
    else
    {
        setError(std::get<1>(returnVal), std::get<2>(returnVal));
        return false;
    }
}

template<class T>
void read8BitRowT(std::istream& stream, std::span<T> textureRow, std::span<const BitmapRGBQuad> colorPalette,
                  std::vector<glm::byte>& pixelBuffer, uint32_t bytesPerRow, uint32_t textureWidth)
{
    stream.read(reinterpret_cast<char*>(pixelBuffer.data()), bytesPerRow);

    for(size_t i = 0; i < textureWidth; ++i)
    {
        textureRow[i] = ColorConverter<T>::convert(colorPalette, pixelBuffer[i]);
    }
}

void BitmapTexImpImporter::read8BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    read8BitRowT(stream, textureRow, colorPalette, mPixelBuffer, mBytesPerRow, mWidth);
}

void BitmapTexImpImporter::read8BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    read8BitRowT(stream, textureRow, colorPalette, mPixelBuffer, mBytesPerRow, mWidth);
}

template<class T>
std::tuple<bool, TextureImportError, const char*>
readRLE8RowT(std::istream& stream, std::span<T> textureRow, std::span<const BitmapRGBQuad> colorPalette,
             std::vector<glm::byte>& pixelBuffer, uint32_t& rowOffset, uint32_t& rowsToSkip, size_t& readOffset,
             uint32_t textureWidth)
{
    rowOffset = 0;
    rowsToSkip = 0;

    stream.read(reinterpret_cast<char*>(pixelBuffer.data()) + readOffset, pixelBuffer.size() - readOffset);
    size_t bytesRead = stream.gcount();
    size_t bufferSize = readOffset + bytesRead;
    std::span<uint8_t> compressedData(pixelBuffer.data(), bufferSize);

    while(!compressedData.empty())
    {
        if(compressedData[0] == 0 && compressedData[1] == 0)
        {
            // end of scan line;
            compressedData = compressedData.subspan(2);

            if(!compressedData.empty())
            {
                std::rotate(std::begin(pixelBuffer), std::begin(pixelBuffer) + (bufferSize - compressedData.size()),
                            pixelBuffer.end());
                readOffset = compressedData.size();
            }
            else { readOffset = 0; }

            return std::make_tuple(true, TextureImportError::None, nullptr);
        }
        else if(compressedData[0] == 0 && compressedData[1] == 1)
        {
            // end of data
            return std::make_tuple(true, TextureImportError::None, nullptr);
        }
        else if(compressedData[0] == 0 && compressedData[1] == 2)
        {
            // move cursor
            uint32_t rowOffsetAbsolute = (textureWidth - static_cast<uint32_t>(textureRow.size()) + compressedData[2]);
            if(compressedData[3] > 0 || rowOffsetAbsolute >= textureWidth)
            {
                rowsToSkip = compressedData[3];

                if(rowOffsetAbsolute < textureWidth) { rowOffset = compressedData[2]; }
                else
                {
                    rowOffset = rowOffsetAbsolute % textureWidth;
                    rowsToSkip += rowOffsetAbsolute / textureWidth;
                }

                compressedData = compressedData.subspan(2);

                if(!compressedData.empty())
                {
                    std::rotate(std::begin(pixelBuffer), std::begin(pixelBuffer) + (bufferSize - compressedData.size()),
                                pixelBuffer.end());
                    readOffset = compressedData.size();
                }
                else { readOffset = 0; }

                return std::make_tuple(true, TextureImportError::None, nullptr);
            }

            textureRow = textureRow.subspan(compressedData[2]);
            compressedData = compressedData.subspan(4);
        }
        else if(compressedData[0] == 0)
        {
            // unencoded run
            size_t count = compressedData[1];
            compressedData = compressedData.subspan(2);

            const size_t textureWriteCount = std::min(count, static_cast<size_t>(textureRow.size()));
            for(size_t i = 0; i < textureWriteCount; ++i)
            {
                uint8_t paletteIndex = compressedData[i];
                textureRow[i] = ColorConverter<T>::convert(colorPalette, paletteIndex);
            }

            if(count > textureWriteCount)
            {
                return std::make_tuple(false, TextureImportError::InvalidDataInImage,
                                       "RLE8 encoded stream ran beyond the end of a row.");
            }

            textureRow = textureRow.subspan(count);
            compressedData = compressedData.subspan(count + (count % 2));
        }
        else
        {
            // repeated color
            const size_t count = compressedData[0];
            T color = ColorConverter<T>::convert(colorPalette, compressedData[1]);

            const size_t textureWriteCount = std::min(count, static_cast<size_t>(textureRow.size()));
            for(size_t i = 0; i < textureWriteCount; ++i)
            {
                textureRow[i] = color;
            }

            if(count > textureWriteCount)
            {
                return std::make_tuple(false, TextureImportError::InvalidDataInImage,
                                       "RLE8 encoded stream ran beyond the end of a row.");
                ;
            }

            textureRow = textureRow.subspan(textureWriteCount);
            compressedData = compressedData.subspan(2);
        }
    }

    return std::make_tuple(true, TextureImportError::None, nullptr);
}

bool BitmapTexImpImporter::readRLE8Row(std::istream& stream, std::span<glm::u8vec3> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    auto returnVal =
        readRLE8RowT(stream, textureRow, colorPalette, mPixelBuffer, mRowOffset, mRowsToSkip, mReadOffset, mWidth);

    if(std::get<0>(returnVal)) { return true; }
    else
    {
        setError(std::get<1>(returnVal), std::get<2>(returnVal));
        return false;
    }
}

bool BitmapTexImpImporter::readRLE8Row(std::istream& stream, std::span<glm::u8vec4> textureRow,
                                       std::span<const BitmapRGBQuad> colorPalette)
{
    auto returnVal =
        readRLE8RowT(stream, textureRow, colorPalette, mPixelBuffer, mRowOffset, mRowsToSkip, mReadOffset, mWidth);

    if(std::get<0>(returnVal)) { return true; }
    else
    {
        setError(std::get<1>(returnVal), std::get<2>(returnVal));
        return false;
    }
}

void BitmapTexImpImporter::read16BitRow_555(std::istream& stream, std::span<uint16_t> textureRow)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    std::span<uint16_t> rawColors(reinterpret_cast<uint16_t*>(mPixelBuffer.data()), mWidth);
    for(uint16_t index = 0; index < rawColors.size(); ++index)
    {
        uint16_t rawColor = rawColors[index];
        textureRow[index] = 0x8000 | rawColor;
    }
}

void BitmapTexImpImporter::read16BitRow_565(std::istream& stream, std::span<uint16_t> textureRow)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    std::span<uint16_t> rawColors(reinterpret_cast<uint16_t*>(mPixelBuffer.data()), mWidth);
    for(uint16_t index = 0; index < rawColors.size(); ++index)
    {
        uint16_t rawColor = rawColors[index];
        textureRow[index] = rawColor;
    }
}

void BitmapTexImpImporter::read16BitRow_RGBMask(std::istream& stream, std::span<glm::u8vec3> textureRow)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    glm::u8vec3 shiftAmount;
    shiftAmount.r = static_cast<uint8_t>(lowestOneBit(mMask.r));
    shiftAmount.g = static_cast<uint8_t>(lowestOneBit(mMask.g));
    shiftAmount.b = static_cast<uint8_t>(lowestOneBit(mMask.b));

    glm::vec3 maxValue;
    maxValue.r = static_cast<float>(mMask.r >> shiftAmount.r);
    maxValue.g = static_cast<float>(mMask.g >> shiftAmount.g);
    maxValue.b = static_cast<float>(mMask.b >> shiftAmount.b);

    std::span<uint16_t> rawColors(reinterpret_cast<uint16_t*>(mPixelBuffer.data()), mWidth);
    for(uint16_t index = 0; index < rawColors.size(); ++index)
    {
        uint16_t rawColor = rawColors[index];
        glm::u8vec3& textureColor = textureRow[index];
        textureColor.r =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.r) >> shiftAmount.r) / maxValue.r * 255.0f));
        textureColor.g =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.g) >> shiftAmount.g) / maxValue.g * 255.0f));
        textureColor.b =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.b) >> shiftAmount.b) / maxValue.b * 255.0f));
    }
}

void BitmapTexImpImporter::read16BitRow_RGBMask(std::istream& stream, std::span<glm::u8vec4> textureRow)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    glm::u8vec3 shiftAmount;
    shiftAmount.r = static_cast<uint8_t>(lowestOneBit(mMask.r));
    shiftAmount.g = static_cast<uint8_t>(lowestOneBit(mMask.g));
    shiftAmount.b = static_cast<uint8_t>(lowestOneBit(mMask.b));

    glm::vec3 maxValue;
    maxValue.r = static_cast<float>(mMask.r >> shiftAmount.r);
    maxValue.g = static_cast<float>(mMask.g >> shiftAmount.g);
    maxValue.b = static_cast<float>(mMask.b >> shiftAmount.b);

    std::span<uint16_t> rawColors(reinterpret_cast<uint16_t*>(mPixelBuffer.data()), mWidth);
    for(uint16_t index = 0; index < rawColors.size(); ++index)
    {
        uint16_t rawColor = rawColors[index];
        glm::u8vec4& textureColor = textureRow[index];
        textureColor.r =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.r) >> shiftAmount.r) / maxValue.r * 255.0f));
        textureColor.g =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.g) >> shiftAmount.g) / maxValue.g * 255.0f));
        textureColor.b =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.b) >> shiftAmount.b) / maxValue.b * 255.0f));
        textureColor.a = 255;
    }
}

void BitmapTexImpImporter::read16BitRow_RGBAMask(std::istream& stream, std::span<glm::u8vec4> textureRow)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    glm::u8vec4 shiftAmount;
    shiftAmount.r = static_cast<uint8_t>(lowestOneBit(mMask.r));
    shiftAmount.g = static_cast<uint8_t>(lowestOneBit(mMask.g));
    shiftAmount.b = static_cast<uint8_t>(lowestOneBit(mMask.b));
    shiftAmount.a = static_cast<uint8_t>(lowestOneBit(mMask.a));

    glm::vec4 maxValue;
    maxValue.r = static_cast<float>(mMask.r >> shiftAmount.r);
    maxValue.g = static_cast<float>(mMask.g >> shiftAmount.g);
    maxValue.b = static_cast<float>(mMask.b >> shiftAmount.b);
    maxValue.a = static_cast<float>(mMask.a >> shiftAmount.a);

    std::span<uint16_t> rawColors(reinterpret_cast<uint16_t*>(mPixelBuffer.data()), mWidth);
    for(uint16_t index = 0; index < rawColors.size(); ++index)
    {
        uint16_t rawColor = rawColors[index];
        glm::u8vec4& textureColor = textureRow[index];
        textureColor.r =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.r) >> shiftAmount.r) / maxValue.r * 255.0f));
        textureColor.g =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.g) >> shiftAmount.g) / maxValue.g * 255.0f));
        textureColor.b =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.b) >> shiftAmount.b) / maxValue.b * 255.0f));
        textureColor.a =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.a) >> shiftAmount.a) / maxValue.a * 255.0f));
    }
}

void BitmapTexImpImporter::read24BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    std::span<glm::u8vec3> rawColors(reinterpret_cast<glm::u8vec3*>(mPixelBuffer.data()), mWidth);
    for(size_t index = 0; index < rawColors.size(); ++index)
    {
        Data24_32 rawColor;
        memcpy(rawColor.bytes24, reinterpret_cast<uint8_t*>(&rawColors[index]), 3);
        glm::u8vec3& textureColor = textureRow[index];
        textureColor.r = static_cast<uint8_t>((rawColor.bytes32 & 0xff0000) >> 16);
        textureColor.g = static_cast<uint8_t>((rawColor.bytes32 & 0x00ff00) >> 8);
        textureColor.b = static_cast<uint8_t>(rawColor.bytes32 & 0x0000ff);
    }
}

void BitmapTexImpImporter::read24BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    std::span<glm::u8vec3> rawColors(reinterpret_cast<glm::u8vec3*>(mPixelBuffer.data()), mWidth);
    for(size_t index = 0; index < rawColors.size(); ++index)
    {
        Data24_32 rawColor;
        memcpy(rawColor.bytes24, reinterpret_cast<uint8_t*>(&rawColors[index]), 3);
        glm::u8vec4& textureColor = textureRow[index];
        textureColor.r = static_cast<uint8_t>((rawColor.bytes32 & 0xff0000) >> 16);
        textureColor.g = static_cast<uint8_t>((rawColor.bytes32 & 0x00ff00) >> 8);
        textureColor.b = static_cast<uint8_t>(rawColor.bytes32 & 0x0000ff);
        textureColor.a = 0xff;
    }
}

void BitmapTexImpImporter::read24BitRow(std::istream& stream, std::span<glm::u8vec3> textureRow,
                                        std::span<const BitmapRGBQuad> colorPalette)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    std::span<glm::u8vec3> paletteIndices(reinterpret_cast<glm::u8vec3*>(mPixelBuffer.data()), mWidth);
    for(size_t index = 0; index < paletteIndices.size(); ++index)
    {
        glm::u8vec3& textureColor = textureRow[index];
        textureColor.r = getColor(colorPalette, paletteIndices[index][2]).red;
        textureColor.g = getColor(colorPalette, paletteIndices[index][1]).green;
        textureColor.b = getColor(colorPalette, paletteIndices[index][0]).blue;
    }
}

void BitmapTexImpImporter::read24BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow,
                                        std::span<const BitmapRGBQuad> colorPalette)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    std::span<glm::u8vec3> paletteIndices(reinterpret_cast<glm::u8vec3*>(mPixelBuffer.data()), mWidth);
    for(size_t index = 0; index < paletteIndices.size(); ++index)
    {
        glm::u8vec4& textureColor = textureRow[index];
        textureColor.r = getColor(colorPalette, paletteIndices[index][2]).red;
        textureColor.g = getColor(colorPalette, paletteIndices[index][1]).green;
        textureColor.b = getColor(colorPalette, paletteIndices[index][0]).blue;
        textureColor.a = 0xff;
    }
}

void BitmapTexImpImporter::read32BitRow(std::istream& stream, std::span<glm::u8vec4> textureRow)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    std::span<glm::u8vec4> colorBuffer(reinterpret_cast<glm::u8vec4*>(mPixelBuffer.data()), mWidth);
    for(size_t index = 0; index < colorBuffer.size(); ++index)
    {
        textureRow[index].r = colorBuffer[index].b;
        textureRow[index].g = colorBuffer[index].g;
        textureRow[index].b = colorBuffer[index].r;
        textureRow[index].a = 255;
    }
}

void BitmapTexImpImporter::read32BitRow_Mask(std::istream& stream, std::span<glm::u8vec4> textureRow)
{
    stream.read(reinterpret_cast<char*>(mPixelBuffer.data()), mBytesPerRow);

    glm::u8vec4 shiftAmount;
    shiftAmount.r = static_cast<uint8_t>(lowestOneBit(mMask.r));
    shiftAmount.g = static_cast<uint8_t>(lowestOneBit(mMask.g));
    shiftAmount.b = static_cast<uint8_t>(lowestOneBit(mMask.b));
    shiftAmount.a = static_cast<uint8_t>(lowestOneBit(mMask.a));

    glm::vec4 maxValue;
    maxValue.r = static_cast<float>(mMask.r >> shiftAmount.r);
    maxValue.g = static_cast<float>(mMask.g >> shiftAmount.g);
    maxValue.b = static_cast<float>(mMask.b >> shiftAmount.b);
    maxValue.a = static_cast<float>(mMask.a >> shiftAmount.a);

    maxValue.r = (maxValue.r > 0) ? maxValue.r : 1.0f;
    maxValue.g = (maxValue.g > 0) ? maxValue.g : 1.0f;
    maxValue.b = (maxValue.b > 0) ? maxValue.b : 1.0f;
    maxValue.a = (maxValue.a > 0) ? maxValue.a : 1.0f;

    uint8_t alphaAdd = (mMask.a > 0) ? 0 : 255;

    std::span<uint32_t> colorBuffer(reinterpret_cast<uint32_t*>(mPixelBuffer.data()), mWidth);
    for(size_t index = 0; index < colorBuffer.size(); ++index)
    {
        uint32_t rawColor = colorBuffer[index];
        glm::u8vec4& textureColor = textureRow[index];
        textureColor.r =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.r) >> shiftAmount.r) / maxValue.r * 255.0f));
        textureColor.g =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.g) >> shiftAmount.g) / maxValue.g * 255.0f));
        textureColor.b =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.b) >> shiftAmount.b) / maxValue.b * 255.0f));
        textureColor.a =
            static_cast<uint8_t>(glm::round(((rawColor & mMask.a) >> shiftAmount.a) / maxValue.a * 255.0f)) + alphaAdd;
    }
}
} // namespace teximp::bitmap

#endif // TEXIMP_ENABLE_BITMAP_BACKEND_TEXIMP