#pragma once

#include <cputex/definitions.h>

#include <cstdint>

namespace teximp::ktx
{
struct header10
{
    uint32_t Endianness;
    uint32_t GLType;
    uint32_t GLTypeSize;
    uint32_t GLFormat;
    uint32_t GLInternalFormat;
    uint32_t GLBaseInternalFormat;
    uint32_t PixelWidth;
    uint32_t PixelHeight;
    uint32_t PixelDepth;
    uint32_t NumberOfArrayElements;
    uint32_t NumberOfFaces;
    uint32_t NumberOfMipmapLevels;
    uint32_t BytesOfKeyValueData;
};

inline cputex::TextureDimension getTextureDimension(header10 const& Header)
{
    if(Header.NumberOfFaces > 1) { return cputex::TextureDimension::TextureCube; }
    else if(Header.PixelHeight == 0) { return cputex::TextureDimension::Texture1D; }
    else if(Header.PixelDepth > 0) { return cputex::TextureDimension::Texture3D; }
    else { return cputex::TextureDimension::Texture2D; }
}
} // namespace teximp::ktx