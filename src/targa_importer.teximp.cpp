#include <teximp/targa/targa_importer.teximp.h>

#ifdef TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP

#include <glm/common.hpp>
#include <gpufmt/utility.h>

namespace teximp::targa
{
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

bool TargaTexImpImporter::readBaseHeader(std::istream& stream)
{
    stream.seekg(0);

    stream.read((char*)(&mIdLength), 1);
    stream.read((char*)(&mColorMapType), 1);
    stream.read((char*)(&mImageType), 1);

    if(!((mImageType >= 0 && mImageType <= 3) || (mImageType >= 9 && mImageType <= 11))) { return false; }

    stream.read((char*)(&mColorMapStartIndex), 2);
    stream.read((char*)(&mColorMapCount), 2);
    stream.read((char*)(&mColorMapEntrySize), 1);

    if(mColorMapType == 1)
    {
        if(mColorMapEntrySize != 8 && mColorMapEntrySize != 15 && mColorMapEntrySize != 16 &&
           mColorMapEntrySize != 24 && mColorMapEntrySize != 32)
        {
            return false;
        }
    }

    stream.read((char*)(&mOriginX), 2);
    stream.read((char*)(&mOriginY), 2);
    stream.read((char*)(&mWidth), 2);

    if(mWidth < 1) { return false; }

    stream.read((char*)(&mHeight), 2);

    if(mHeight < 1) { return false; }

    stream.read((char*)(&mBitsPerPixel), 1);
    stream.read((char*)(&mImageDescriptor), 1);

    if(mColorMapType == 1 && (mBitsPerPixel != 8) && (mBitsPerPixel != 16)) { return false; }

    if(mBitsPerPixel != 8 && mBitsPerPixel != 15 && mBitsPerPixel != 16 && mBitsPerPixel != 24 && mBitsPerPixel != 32)
    {
        return false;
    }

    return true;
}

bool isColorMap(uint8_t imageType)
{
    return imageType == 1 || imageType == 9;
}

bool isTrueColor(uint8_t imageType)
{
    return imageType == 2 || imageType == 10;
}

bool isGreyScale(uint8_t imageType)
{
    return imageType == 3 || imageType == 11;
}

bool isRLECompressed(uint8_t imageType)
{
    return imageType >= 9 && imageType <= 11;
}

void TargaTexImpImporter::load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options)
{
    if(!mFooterFound) { readBaseHeader(stream); }

    mImageOrigin = (ImageOrigin)((mImageDescriptor & 0b110000) >> 4);
    mNumAlphaBits = mImageDescriptor & 0xf;

    if(mIdLength > 0)
    {
        mImageId.resize(mIdLength);
        stream.read((char*)(mImageId.data()), mIdLength);
    }

    if(mColorMapType == 1)
    {
        mColorMapData.resize(mColorMapCount * (mColorMapEntrySize / 8));
        stream.read((char*)(mColorMapData.data()), mColorMapData.size());
    }

    if(mImageType == 0)
    {
        setError(TextureImportError::InvalidDataInImage, "Not image data in file.");
        return;
    }

    if(mImageType != 1 ||  // uncompressed color map
       mImageType != 2 ||  // uncompressed true color
       mImageType != 3 ||  // uncompressed grayscale
       mImageType != 9 ||  // RLE color map
       mImageType != 10 || // RLE true color
       mImageType != 11)   // RLE grayscale
    {
        setError(TextureImportError::InvalidDataInImage,
                 std::format("Image type {} is not a valid image type.", mImageType));
        return;
    }

    gpufmt::Format format = gpufmt::Format::B8G8R8A8_UNORM;

    uint32_t uncompressedBitsPerPixel;

    if(isTrueColor(mImageType)) { uncompressedBitsPerPixel = mBitsPerPixel; }
    else if(isColorMap(mImageType)) { uncompressedBitsPerPixel = mColorMapEntrySize; }
    else { uncompressedBitsPerPixel = mBitsPerPixel; }

    switch(uncompressedBitsPerPixel)
    {
    case 8: format = gpufmt::Format::R8_UNORM; break;
    case 15: format = gpufmt::Format::R5G5B5A1_UNORM_PACK16; break;
    case 16: format = gpufmt::Format::R5G5B5A1_UNORM_PACK16; break;
    case 24: format = gpufmt::Format::B8G8R8_UNORM; break;
    case 32: format = gpufmt::Format::B8G8R8A8_UNORM; break;
    }

    if(options.assumeSrgb) { format = gpufmt::sRGBFormat(format).value_or(format); }

    cputex::TextureParams textureParams{
        .format = format,
        .dimension = cputex::TextureDimension::Texture2D,
        .extent = {mWidth, mHeight, 1},
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

    std::span<std::byte> surfaceSpan = textureAllocator.accessTextureData(0, {});

    // read image
    switch(mImageType)
    {
    case 1:
        // uncompressed color map
        readColorMap(stream, surfaceSpan);
        break;
    case 2:
        // uncompressed true color
        readTrueColor(stream, surfaceSpan);
        break;
    case 3:
        // uncompressed grayscale
        readGrayScale(stream, surfaceSpan);
        break;
    case 9:
        // RLE color map
        readColorMapRLE(stream, surfaceSpan);
        break;
    case 10:
        // RLE true color
        readTrueColorRLE(stream, surfaceSpan);
        break;
    case 11:
        // RLE grayscale
        readTrueColorRLE(stream, surfaceSpan);
        break;
    }
}

void TargaTexImpImporter::readTrueColor(std::istream& stream, std::span<std::byte> surfaceSpan)
{
    uint32_t bytesPerPixel;

    if(mBitsPerPixel == 15) { bytesPerPixel = 2; }
    else { bytesPerPixel = mBitsPerPixel / 8; }

    uint32_t rowPitch = mWidth * bytesPerPixel;

    if(mImageOrigin == ImageOrigin::LowerLeft)
    {
        for(int32_t row = mHeight - 1; row >= 0; --row)
        {
            stream.read(castWritableBytes<char>(surfaceSpan.subspan(row * rowPitch)).data(), rowPitch);
        }
    }
    else
    {
        for(int32_t row = 0; row <= mHeight; ++row)
        {
            stream.read(castWritableBytes<char>(surfaceSpan.subspan(row * rowPitch)).data(), rowPitch);
        }
    }
}

template<class ColorT>
void readTrueColorRLEGeneric(std::istream& stream, std::span<ColorT> textureData)
{
    static constexpr uint32_t bytesPerPixel = sizeof(ColorT);

    std::vector<uint8_t> rleData;

    while(!textureData.empty())
    {
        uint8_t headerByte;
        stream.read((char*)(&headerByte), 1);

        uint32_t count = glm::min((uint32_t)textureData.size(), (uint32_t)(headerByte & 0b0111'1111) + 1);

        if((headerByte & 0b1000'0000) == 0b1000'0000)
        {
            // repeated data
            ColorT color;
            stream.read((char*)&color, bytesPerPixel);
            std::fill_n(textureData.begin(), count, color);
        }
        else
        {
            // uncompressed data
            rleData.resize(count * bytesPerPixel);
            stream.read((char*)rleData.data(), count * bytesPerPixel);
            memcpy(textureData.data(), rleData.data(), count * bytesPerPixel);
        }

        textureData = textureData.subspan(count);
    }
}

void TargaTexImpImporter::readTrueColorRLE(std::istream& stream, std::span<std::byte> surfaceSpan)
{
    uint32_t bytesPerPixel;

    if(mBitsPerPixel == 15) { bytesPerPixel = 2; }
    else { bytesPerPixel = mBitsPerPixel / 8; }

    switch(bytesPerPixel)
    {
    case 1: readTrueColorRLEGeneric(stream, castWritableBytes<uint8_t>(surfaceSpan)); break;
    case 2: readTrueColorRLEGeneric(stream, castWritableBytes<uint16_t>(surfaceSpan)); break;
    case 3: readTrueColorRLEGeneric(stream, castWritableBytes<glm::u8vec3>(surfaceSpan)); break;
    case 4: readTrueColorRLEGeneric(stream, castWritableBytes<uint32_t>(surfaceSpan)); break;
    }

    if(mImageOrigin == ImageOrigin::LowerLeft)
    {
        uint32_t rowPitch = mWidth * bytesPerPixel;
        std::vector<uint8_t> tempRow;
        tempRow.resize(rowPitch);

        for(size_t row = 0; row < mHeight / 2; ++row)
        {
            std::memcpy(tempRow.data(), surfaceSpan.subspan(row * rowPitch).data(), rowPitch);
            std::memcpy(surfaceSpan.subspan(row * rowPitch).data(),
                        surfaceSpan.subspan((mHeight - row - 1) * rowPitch).data(), rowPitch);
            std::memcpy(surfaceSpan.subspan((mHeight - row - 1) * rowPitch).data(), tempRow.data(), rowPitch);
        }
    }
}

template<class ColorT, class IndexT>
void readColorMapGeneric(std::istream& stream, std::span<ColorT> textureData, uint32_t width, uint32_t height,
                         uint32_t yDirection, std::span<const ColorT> colorMap)
{
    int32_t yStart = 0;
    int32_t yEnd;
    if(yDirection == 1)
    {
        yStart = 0;
        yEnd = height;
    }
    else
    {
        yStart = height - 1;
        yEnd = -1;
    }

    for(int32_t y = yStart; y != yEnd; y += yDirection)
    {
        for(int32_t x = 0; x < (int32_t)width; ++x)
        {
            IndexT index;
            stream.read((char*)(&index), sizeof(IndexT));

            ColorT color = colorMap[index];
            textureData[y * width + x] = color;
        }
    }
}

void TargaTexImpImporter::readColorMap(std::istream& stream, std::span<std::byte> surfaceSpan)
{
    uint32_t bytesPerPixel = (mColorMapEntrySize + 7) / 8;
    uint32_t indexSizeInBytes = (mBitsPerPixel + 7) / 8;
    int32_t yDirection = 1;

    if(mImageOrigin == ImageOrigin::LowerLeft || mImageOrigin == ImageOrigin::LowerRight) { yDirection = -1; }

    if(bytesPerPixel == 1 && indexSizeInBytes == 1)
    {
        readColorMapGeneric<uint8_t, uint8_t>(stream, castWritableBytes<uint8_t>(surfaceSpan), mWidth, mHeight,
                                              yDirection,
                                              std::span<const uint8_t>(mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 1 && indexSizeInBytes == 2)
    {
        readColorMapGeneric<uint8_t, uint16_t>(stream, castWritableBytes<uint8_t>(surfaceSpan), mWidth, mHeight,
                                               yDirection,
                                               std::span<const uint8_t>(mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 2 && indexSizeInBytes == 1)
    {
        readColorMapGeneric<uint16_t, uint8_t>(
            stream, castWritableBytes<uint16_t>(surfaceSpan), mWidth, mHeight, yDirection,
            std::span<const uint16_t>((uint16_t*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 2 && indexSizeInBytes == 2)
    {
        readColorMapGeneric<uint16_t, uint16_t>(
            stream, castWritableBytes<uint16_t>(surfaceSpan), mWidth, mHeight, yDirection,
            std::span<const uint16_t>((uint16_t*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 3 && indexSizeInBytes == 1)
    {
        readColorMapGeneric<glm::u8vec3, uint8_t>(
            stream, castWritableBytes<glm::u8vec3>(surfaceSpan), mWidth, mHeight, yDirection,
            std::span<const glm::u8vec3>((glm::u8vec3*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 3 && indexSizeInBytes == 2)
    {
        readColorMapGeneric<glm::u8vec3, uint16_t>(
            stream, castWritableBytes<glm::u8vec3>(surfaceSpan), mWidth, mHeight, yDirection,
            std::span<const glm::u8vec3>((glm::u8vec3*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 4 && indexSizeInBytes == 1)
    {
        readColorMapGeneric<uint32_t, uint8_t>(
            stream, castWritableBytes<uint32_t>(surfaceSpan), mWidth, mHeight, yDirection,
            std::span<const uint32_t>((uint32_t*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 4 && indexSizeInBytes == 2)
    {
        readColorMapGeneric<uint32_t, uint16_t>(
            stream, castWritableBytes<uint32_t>(surfaceSpan), mWidth, mHeight, yDirection,
            std::span<const uint32_t>((uint32_t*)mColorMapData.data(), mColorMapCount));
    }
}

template<class ColorT, class IndexT>
void readColorMapRLEGeneric(std::istream& stream, std::span<ColorT> textureData, std::span<const ColorT> colorMap)
{
    std::vector<uint8_t> rleData;

    while(!textureData.empty())
    {
        uint8_t headerByte;
        stream.read((char*)(&headerByte), 1);

        uint32_t count = glm::min((uint32_t)textureData.size(), (uint32_t)(headerByte & 0b0111'1111) + 1);

        if((headerByte & 0b1000'0000) == 0b1000'0000)
        {
            // repeated data

            IndexT index = 0;
            stream.read((char*)&index, sizeof(IndexT));

            ColorT color = colorMap[index];
            std::fill_n(textureData.begin(), count, color);
        }
        else
        {
            // uncompressed data
            rleData.resize(count * sizeof(IndexT));
            stream.read((char*)rleData.data(), count * sizeof(IndexT));

            std::span<IndexT> indices((IndexT*)rleData.data(), count);

            ptrdiff_t textureIndex = 0;
            for(const IndexT index : indices)
            {
                ColorT color = colorMap[index];
                textureData[textureIndex++] = color;
            }
        }

        textureData = textureData.subspan(count);
    }
}

void TargaTexImpImporter::readColorMapRLE(std::istream& stream, std::span<std::byte> surfaceSpan)
{
    uint32_t bytesPerPixel = (mColorMapEntrySize + 7) / 8;
    uint32_t indexSizeInBytes = mBitsPerPixel / 8;

    if(bytesPerPixel == 1 && indexSizeInBytes == 1)
    {
        readColorMapRLEGeneric<uint8_t, uint8_t>(stream, castWritableBytes<uint8_t>(surfaceSpan),
                                                 std::span<const uint8_t>(mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 1 && indexSizeInBytes == 2)
    {
        readColorMapRLEGeneric<uint8_t, uint16_t>(stream, castWritableBytes<uint8_t>(surfaceSpan),
                                                  std::span<const uint8_t>(mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 2 && indexSizeInBytes == 1)
    {
        readColorMapRLEGeneric<uint16_t, uint8_t>(
            stream, castWritableBytes<uint16_t>(surfaceSpan),
            std::span<const uint16_t>((uint16_t*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 2 && indexSizeInBytes == 2)
    {
        readColorMapRLEGeneric<uint16_t, uint16_t>(
            stream, castWritableBytes<uint16_t>(surfaceSpan),
            std::span<const uint16_t>((uint16_t*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 3 && indexSizeInBytes == 1)
    {
        readColorMapRLEGeneric<glm::u8vec3, uint8_t>(
            stream, castWritableBytes<glm::u8vec3>(surfaceSpan),
            std::span<const glm::u8vec3>((glm::u8vec3*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 3 && indexSizeInBytes == 2)
    {
        readColorMapRLEGeneric<glm::u8vec3, uint16_t>(
            stream, castWritableBytes<glm::u8vec3>(surfaceSpan),
            std::span<const glm::u8vec3>((glm::u8vec3*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 4 && indexSizeInBytes == 1)
    {
        readColorMapRLEGeneric<uint32_t, uint8_t>(
            stream, castWritableBytes<uint32_t>(surfaceSpan),
            std::span<const uint32_t>((uint32_t*)mColorMapData.data(), mColorMapCount));
    }
    else if(bytesPerPixel == 4 && indexSizeInBytes == 2)
    {
        readColorMapRLEGeneric<uint32_t, uint16_t>(
            stream, castWritableBytes<uint32_t>(surfaceSpan),
            std::span<const uint32_t>((uint32_t*)mColorMapData.data(), mColorMapCount));
    }

    uint32_t rowPitch = mWidth * bytesPerPixel;

    std::vector<uint8_t> tempRow;
    tempRow.resize(rowPitch);

    if(mImageOrigin == ImageOrigin::LowerLeft)
    {
        tempRow.resize(rowPitch);
        for(size_t row = 0; row < mHeight / 2; ++row)
        {
            memcpy(tempRow.data(), surfaceSpan.subspan(row * rowPitch).data(), rowPitch);
            memcpy(surfaceSpan.subspan(row * rowPitch).data(),
                   surfaceSpan.subspan((mHeight - row - 1) * rowPitch).data(), rowPitch);
            memcpy(surfaceSpan.subspan((mHeight - row - 1) * rowPitch).data(), tempRow.data(), rowPitch);
        }
    }
}

void TargaTexImpImporter::readGrayScale(std::istream& stream, std::span<std::byte> surfaceSpan)
{
    ptrdiff_t rowPitch = mWidth * ((mBitsPerPixel + 7) / 8);

    if(mImageOrigin == ImageOrigin::UpperLeft)
    {
        for(uint32_t row = 0; row < mHeight; ++row)
        {
            stream.read(castWritableBytes<char>(surfaceSpan.subspan(row * rowPitch)).data(), rowPitch);
        }
    }
    else if(mImageOrigin == ImageOrigin::LowerLeft)
    {
        for(int32_t row = mHeight - 1; row >= 0; --row)
        {
            stream.read(castWritableBytes<char>(surfaceSpan.subspan(row * rowPitch)).data(), rowPitch);
        }
    }
}
} // namespace teximp::targa

#endif // TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP