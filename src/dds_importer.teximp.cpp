#include <teximp/dds/dds_importer.teximp.h>

#ifdef TEXIMP_ENABLE_DDS_BACKEND_TEXIMP

#include <cputex/unique_texture.h>
#include <cputex/utility.h>
#include <glm/glm.hpp>
#include <gpufmt/dxgi.h>
#include <gpufmt/traits.h>
#include <gpufmt/utility.h>

#include <bitset>
#include <format>

namespace teximp::dds
{
FileFormat DdsTexImpImporter::fileFormat() const
{
    return FileFormat::Dds;
}

const dds::DDS_HEADER& DdsTexImpImporter::header() const
{
    return mHeader;
}

const dds::DDS_HEADER_DXT10& DdsTexImpImporter::header10() const
{
    return mHeader10;
}

bool DdsTexImpImporter::checkSignature(std::istream& stream)
{
    char fourcc[4];
    stream.read(fourcc, sizeof(dds::DDS_MAGIC));

    if(!stream.good()) { return false; }

    if(std::memcmp(fourcc, &dds::DDS_MAGIC, 4) != 0) { return false; }

    return true;
}

cputex::TextureDimension getTextureDimension(const dds::DDS_HEADER& header, const dds::DDS_HEADER_DXT10& header10)
{
    if(header.caps2 & dds::DDSCAPS2_CUBEMAP) { return cputex::TextureDimension::TextureCube; }
    else if((header.caps2 & dds::DDSCAPS2_VOLUME) || (header.flags & dds::DDSD_DEPTH) ||
            header10.resourceDimension == dds::DDS_DIMENSION_TEXTURE3D)
    {
        return cputex::TextureDimension::Texture3D;
    }
    else if(header10.resourceDimension == dds::DDS_DIMENSION_TEXTURE1D || (header.flags & dds::DDSD_HEIGHT) == 0u)
    {
        return cputex::TextureDimension::Texture1D;
    }
    else if(header10.resourceDimension == dds::DDS_DIMENSION_TEXTURE2D)
    {
        if((header10.miscFlag & dds::DDS_RESOURCE_MISC_TEXTURECUBE)) { return cputex::TextureDimension::TextureCube; }
        else { return cputex::TextureDimension::Texture2D; }
    }
    else { return cputex::TextureDimension::Texture2D; }
}

void DdsTexImpImporter::load(std::istream& stream, ITextureAllocator& textureAllocator,
                             TextureImportOptions /*options*/)
{
    stream.read(reinterpret_cast<char*>(&mHeader), sizeof(dds::DDS_HEADER));

    if(stream.fail())
    {
        setError(TextureImportError::CouldNotReadHeader, "Not enough bytes in file for the dds header.");
        return;
    }

    gpufmt::Format srcFormat = gpufmt::Format::UNDEFINED;

    if(mHeader.format.flags & dds::DDPF_FOURCC)
    {
        if(mHeader.format.fourCC == dds::DDSPF_DX10.fourCC)
        {
            stream.read(reinterpret_cast<char*>(&mHeader10), sizeof(dds::DDS_HEADER_DXT10));

            if(stream.fail())
            {
                setError(TextureImportError::CouldNotReadHeader, "Not enough bytes in file for the dds dx10 header.");
                return;
            }

            auto translatedFormat = gpufmt::dxgi::translateFormat(mHeader10.dxgiFormat);

            if(!translatedFormat)
            {
                setError(TextureImportError::UnknownFormat,
                         std::format("Unsupported format '{}' ({})", gpufmt::toString(mHeader10.dxgiFormat),
                                     static_cast<uint32_t>(mHeader10.dxgiFormat)));
                return;
            }

            srcFormat = translatedFormat.value();
        }
        else
        {
            srcFormat = dds::fourCCToFormat(mHeader.format.fourCC);

            if(srcFormat == gpufmt::Format::UNDEFINED)
            {
                if(mHeader.format.fourCC < 200)
                {
                    setError(TextureImportError::UnknownFormat,
                             std::format("Unsupported fourcc format '{}' ({})",
                                         dds::to_string(static_cast<dds::D3DFORMAT>(mHeader.format.fourCC)),
                                         mHeader.format.fourCC));
                    return;
                }
                else
                {
                    setError(TextureImportError::UnknownFormat,
                             std::format("Unsupported fourcc format '{}' ({})",
                                         std::string_view(reinterpret_cast<const char*>(&mHeader.format.fourCC), 4),
                                         mHeader.format.fourCC));
                    return;
                }
            }
        }
    }
    else if(mHeader.format.flags & (dds::DDPF_ALPHA | dds::DDPF_ALPHAPIXELS | dds::DDPF_RGB | dds::DDPF_RGBA))
    {
        if(mHeader.format.rgbBitCount == 0)
        {
            setError(TextureImportError::InvalidDataInImage,
                     "Format flags require bits per pixel (bpp) to be greater than 0.");
            return;
        }

        for(gpufmt::Format format : gpufmt::FormatEnumerator())
        {
            const auto& formatInfo = gpufmt::formatInfo(format);

            if(formatInfo.blockByteSize * 8u == mHeader.format.rgbBitCount && !formatInfo.isSigned &&
               formatInfo.redBitMask.mask == mHeader.format.rBitMask &&
               formatInfo.greenBitMask.mask == mHeader.format.gBitMask &&
               formatInfo.blueBitMask.mask == mHeader.format.bBitMask &&
               formatInfo.alphaBitMask.mask == mHeader.format.aBitMask)
            {
                if(gpufmt::dxgi::translateFormat(format))
                {
                    srcFormat = format;
                    break;
                }
            }
        }
    }
    else if(mHeader.format.flags & (dds::DDPF_LUMINANCE | dds::DDPF_LUMINANCE_ALPHA)) {}
    else if(mHeader.format.flags & dds::DDPF_BUMPDUDV)
    {
        if(mHeader.format.rgbBitCount == 0)
        {
            setError(TextureImportError::InvalidDataInImage,
                     "Format flags require bits per pixel (bpp) to be greater than 0.");
            return;
        }

        for(gpufmt::Format format : gpufmt::FormatEnumerator())
        {
            const auto& formatInfo = gpufmt::formatInfo(format);

            if(formatInfo.blockByteSize * 8u == mHeader.format.rgbBitCount && formatInfo.isSigned &&
               formatInfo.redBitMask.mask == mHeader.format.rBitMask &&
               formatInfo.greenBitMask.mask == mHeader.format.gBitMask &&
               formatInfo.blueBitMask.mask == mHeader.format.bBitMask &&
               formatInfo.alphaBitMask.mask == mHeader.format.aBitMask)
            {
                if(gpufmt::dxgi::translateFormat(format))
                {
                    srcFormat = format;
                    break;
                }
            }
        }
    }

    if(srcFormat == gpufmt::Format::UNDEFINED)
    {
        setError(TextureImportError::Unknown, "Invalid dxgi format");
        return;
    }

    uint32_t const mips = (mHeader.flags & dds::DDSD_MIPMAPCOUNT) ? mHeader.mipMapCount : 1u;
    uint32_t faces = (mHeader.caps2 & dds::DDSCAPS2_CUBEMAP) ? 6u : 1u;
    uint32_t depthCount = (mHeader.caps2 & dds::DDSCAPS2_VOLUME) ? mHeader.depth : 1u;

    textureAllocator.preAllocation(1);

    cputex::TextureParams params;
    params.dimension = getTextureDimension(mHeader, mHeader10);
    params.extent = {mHeader.width, mHeader.height, depthCount};
    params.faces = faces;
    params.mips = mips;
    params.arraySize = std::max(mHeader10.arraySize, 1u);
    params.format = srcFormat;

    if(!textureAllocator.allocateTexture(params, 0))
    {
        setTextureAllocationError(params);
        return;
    }

    textureAllocator.postAllocation();

    const gpufmt::FormatInfo& formatInfo = gpufmt::formatInfo(params.format);

    auto textureDataPos = stream.tellg();
    stream.seekg(0, std::ios_base::end);
    auto endPos = stream.tellg();
    stream.seekg(textureDataPos);

    for(cputex::CountType slice = 0; slice < params.arraySize; ++slice)
    {
        for(cputex::CountType face = 0; face < params.faces; ++face)
        {
            for(cputex::CountType mip = 0; mip < params.mips; ++mip)
            {
                if(params.dimension == cputex::TextureDimension::TextureCube &&
                   (mHeader.caps2 & dds::DDS_CUBEMAP_ALLFACES))
                {
                    if(face == 0 && (mHeader.caps2 & dds::DDS_CUBEMAP_POSITIVEX) == 0) { continue; }
                    else if(face == 1 && (mHeader.caps2 & dds::DDS_CUBEMAP_NEGATIVEX) == 0) { continue; }
                    else if(face == 2 && (mHeader.caps2 & dds::DDS_CUBEMAP_POSITIVEY) == 0) { continue; }
                    else if(face == 3 && (mHeader.caps2 & dds::DDS_CUBEMAP_NEGATIVEY) == 0) { continue; }
                    else if(face == 4 && (mHeader.caps2 & dds::DDS_CUBEMAP_POSITIVEZ) == 0) { continue; }
                    else if(face == 5 && (mHeader.caps2 & dds::DDS_CUBEMAP_NEGATIVEZ) == 0) { continue; }
                }

                std::span<std::byte> surface = textureAllocator.accessTextureData(
                    0, MipSurfaceKey{.arraySlice = (int16_t)slice, .face = (int8_t)face, .mip = (int8_t)mip});

                const cputex::Extent mipExtent = cputex::calculateMipExtent(params.extent, mip);
                cputex::Extent mipBlockExtent = mipExtent / formatInfo.blockExtent;
                mipBlockExtent = glm::max(mipBlockExtent, {1, 1, 1});

                size_t surfaceSize = mipBlockExtent.x * mipBlockExtent.y * mipBlockExtent.z * formatInfo.blockByteSize;
                stream.read(reinterpret_cast<char*>(surface.data()), surfaceSize);

                if(stream.fail())
                {
                    const size_t actualSize = endPos - textureDataPos;
                    setError(TextureImportError::NotEnoughData,
                             std::format("Prematurely reached the end of the file. Expected the file to have {} bytes "
                                         "of texture data, but only had {} bytes.",
                                         surface.size_bytes(), actualSize));

                    return;
                }
            }
        }
    }
}
} // namespace teximp::dds

#endif // TEXIMP_ENABLE_DDS_BACKEND_TEXIMP