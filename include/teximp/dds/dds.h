#pragma once

#include <gpufmt/dxgi.h>

#include <array>

namespace teximp::dds
{
constexpr uint32_t makeFourCC(uint8_t cc0, uint8_t cc1, uint8_t cc2, uint8_t cc3)
{
    return ((static_cast<uint32_t>(cc3) << 24u) & 0xff000000) | ((static_cast<uint32_t>(cc2) << 16u) & 0x00ff0000) |
           ((static_cast<uint32_t>(cc1) << 8u) & 0x0000ff00) | (static_cast<uint32_t>(cc0) & 0x000000ff);
}

static constexpr uint32_t DDS_MAGIC = makeFourCC('D', 'D', 'S', ' ');

enum DDPF
{
    DDPF_ALPHAPIXELS = 0x00000001u,
    DDPF_ALPHA = 0x00000002u,
    DDPF_FOURCC = 0x00000004u,
    DDPF_PAL8 = 0x00000020u,
    DDPF_PALA = DDPF_PAL8 | DDPF_ALPHAPIXELS,
    DDPF_RGB = 0x00000040u,
    DDPF_RGBA = DDPF_RGB | DDPF_ALPHAPIXELS,
    DDPF_LUMINANCE = 0x00020000u,
    DDPF_LUMINANCE_ALPHA = DDPF_LUMINANCE | DDPF_ALPHAPIXELS,
    DDPF_BUMPDUDV = 0x00080000u
};

struct DDS_PIXELFORMAT
{
    uint32_t size;
    DDPF flags;
    uint32_t fourCC;
    uint32_t rgbBitCount = 0u;
    uint32_t rBitMask = 0u;
    uint32_t gBitMask = 0u;
    uint32_t bBitMask = 0u;
    uint32_t aBitMask = 0u;
};

enum DDSD
{
    DDSD_CAPS = 0x1,
    DDSD_HEIGHT = 0x2,
    DDSD_WIDTH = 0x4,
    DDSD_PITCH = 0x8,
    DDSD_PIXELFORMAT = 0x1000,
    DDSD_MIPMAPCOUNT = 0x20000,
    DDSD_LINEARSIZE = 0x80000,
    DDSD_DEPTH = 0x800000,
};

static constexpr uint32_t DDS_HEADER_FLAGS_TEXTURE = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
static constexpr uint32_t DDS_HEADER_FLAGS_MIPMAP = DDSD_MIPMAPCOUNT;
static constexpr uint32_t DDS_HEADER_FLAGS_VOLUME = DDSD_DEPTH;
static constexpr uint32_t DDS_HEADER_FLAGS_PITCH = DDSD_PITCH;
static constexpr uint32_t DDS_HEADER_FLAGS_LINEARSIZE = DDSD_LINEARSIZE;

enum DDSCAPS
{
    DDSCAPS_COMPLEX = 0x8,
    DDSCAPS_MIPMAP = 0x400000,
    DDSCAPS_TEXTURE = 0x1000
};

static constexpr uint32_t DDS_SURFACE_FLAGS_MIPMAP = DDSCAPS_COMPLEX | DDSCAPS_MIPMAP;
static constexpr uint32_t DDS_SURFACE_FLAGS_TEXTURE = DDSCAPS_TEXTURE;
static constexpr uint32_t DDS_SURFACE_FLAGS_CUBEMAP = DDSCAPS_COMPLEX;

enum DDSCAPS2
{
    DDSCAPS2_CUBEMAP = 0x200,
    DDSCAPS2_CUBEMAP_POSITIVEX = 0x400,
    DDSCAPS2_CUBEMAP_NEGATIVEX = 0x800,
    DDSCAPS2_CUBEMAP_POSITIVEY = 0x1000,
    DDSCAPS2_CUBEMAP_NEGATIVEY = 0x2000,
    DDSCAPS2_CUBEMAP_POSITIVEZ = 0x4000,
    DDSCAPS2_CUBEMAP_NEGATIVEZ = 0x8000,
    DDSCAPS2_VOLUME = 0x200000,
};

static constexpr uint32_t DDS_CUBEMAP_POSITIVEX = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEX;
static constexpr uint32_t DDS_CUBEMAP_NEGATIVEX = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEX;
static constexpr uint32_t DDS_CUBEMAP_POSITIVEY = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEY;
static constexpr uint32_t DDS_CUBEMAP_NEGATIVEY = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEY;
static constexpr uint32_t DDS_CUBEMAP_POSITIVEZ = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_POSITIVEZ;
static constexpr uint32_t DDS_CUBEMAP_NEGATIVEZ = DDSCAPS2_CUBEMAP | DDSCAPS2_CUBEMAP_NEGATIVEZ;
static constexpr uint32_t DDS_CUBEMAP_ALLFACES = DDS_CUBEMAP_POSITIVEX | DDS_CUBEMAP_NEGATIVEX | DDS_CUBEMAP_POSITIVEY |
                                                 DDS_CUBEMAP_NEGATIVEY | DDS_CUBEMAP_POSITIVEZ |
                                                 DDSCAPS2_CUBEMAP_NEGATIVEZ;
static constexpr uint32_t DDS_FLAGS_VOLUME = DDSCAPS2_VOLUME;

struct DDS_HEADER
{
    uint32_t size;
    uint32_t flags;
    uint32_t height;
    uint32_t width;
    uint32_t pitchOrLinearSize;
    uint32_t depth;
    uint32_t mipMapCount;
    std::array<uint32_t, 11> reserved1;
    DDS_PIXELFORMAT format;
    uint32_t caps;
    uint32_t caps2;
    uint32_t caps3;
    uint32_t caps4;
    uint32_t reserved2;
};

static_assert(sizeof(DDS_HEADER) == 124);

enum DDS_RESOURCE_DIMENSION
{
    DDS_DIMENSION_UNKNOWN = 0,
    DDS_DIMENSION_BUFFER = 1,
    DDS_DIMENSION_TEXTURE1D = 2,
    DDS_DIMENSION_TEXTURE2D = 3,
    DDS_DIMENSION_TEXTURE3D = 4,
};

enum DDS_RESOURCE_MISC_FLAG
{
    DDS_RESOURCE_MISC_TEXTURECUBE = 0x4L,
};

enum DDS_MISC_FLAGS2
{
    DDS_MISC_FLAGS2_ALPHA_MODE_MASK = 0x7L,
};

enum DDS_ALPHA_MODE : uint32_t
{
    DDS_ALPHA_MODE_UNKNOWN = 0,
    DDS_ALPHA_MODE_STRAIGHT = 1,
    DDS_ALPHA_MODE_PREMULTIPLIED = 2,
    DDS_ALPHA_MODE_OPAQUE = 3,
    DDS_ALPHA_MODE_CUSTOM = 4,
};

struct DDS_HEADER_DXT10
{
    DXGI_FORMAT dxgiFormat;
    DDS_RESOURCE_DIMENSION resourceDimension;
    uint32_t miscFlag;
    uint32_t arraySize;
    uint32_t miscFlags2;
};

static_assert(sizeof(DDS_HEADER_DXT10) == 20);

constexpr DDS_PIXELFORMAT DDSPF_DXT1{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('D', 'X', 'T', '1')};
constexpr DDS_PIXELFORMAT DDSPF_DXT2{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('D', 'X', 'T', '2')};
constexpr DDS_PIXELFORMAT DDSPF_DXT3{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('D', 'X', 'T', '3')};
constexpr DDS_PIXELFORMAT DDSPF_DXT4{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('D', 'X', 'T', '4')};
constexpr DDS_PIXELFORMAT DDSPF_DXT5{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('D', 'X', 'T', '5')};
constexpr DDS_PIXELFORMAT DDSPF_BC4U{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('B', 'C', '4', 'U')};
constexpr DDS_PIXELFORMAT DDSPF_BC4S{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('B', 'C', '4', 'S')};
constexpr DDS_PIXELFORMAT DDSPF_BC5U{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('B', 'C', '5', 'U')};
constexpr DDS_PIXELFORMAT DDSPF_BC5S{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('B', 'C', '5', 'S')};
constexpr DDS_PIXELFORMAT DDSPF_RGBG{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('R', 'G', 'B', 'G')};
constexpr DDS_PIXELFORMAT DDSPF_GRGB{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('G', 'R', 'G', 'B')};
constexpr DDS_PIXELFORMAT DDSPF_YUY2{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('Y', 'U', 'Y', '2')};
constexpr DDS_PIXELFORMAT DDSPF_ATI1{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('A', 'T', 'I', '1')}; // BC4_UNORM
constexpr DDS_PIXELFORMAT DDSPF_ATI2{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('A', 'T', 'I', '2')}; // BC4_UNORM
constexpr DDS_PIXELFORMAT DDSPF_BC6H{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('B', 'C', '6', 'H')};
constexpr DDS_PIXELFORMAT DDSPF_BC7L{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('B', 'C', '7', 'L')};
constexpr DDS_PIXELFORMAT DDSPF_EAR{.size = sizeof(DDS_PIXELFORMAT),
                                    .flags = DDPF_FOURCC,
                                    .fourCC = makeFourCC('E', 'A', 'R', ' ')};
constexpr DDS_PIXELFORMAT DDSPF_EARG{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('E', 'A', 'R', 'G')};
constexpr DDS_PIXELFORMAT DDSPF_ET2{.size = sizeof(DDS_PIXELFORMAT),
                                    .flags = DDPF_FOURCC,
                                    .fourCC = makeFourCC('E', 'T', '2', ' ')};
constexpr DDS_PIXELFORMAT DDSPF_ET2A{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('E', 'T', '2', 'A')};

constexpr DDS_PIXELFORMAT DDSPF_A8R8G8B8{.size = sizeof(DDS_PIXELFORMAT),
                                         .flags = DDPF_RGBA,
                                         .fourCC = 0u,
                                         .rgbBitCount = 32u,
                                         .rBitMask = 0x00ff0000,
                                         .gBitMask = 0x0000ff00,
                                         .bBitMask = 0x000000ff,
                                         .aBitMask = 0xff000000};
constexpr DDS_PIXELFORMAT DDSPF_X8R8G8B8{.size = sizeof(DDS_PIXELFORMAT),
                                         .flags = DDPF_RGB,
                                         .fourCC = 0u,
                                         .rgbBitCount = 32u,
                                         .rBitMask = 0x00ff0000,
                                         .gBitMask = 0x0000ff00,
                                         .bBitMask = 0x000000ff,
                                         .aBitMask = 0x00000000};
constexpr DDS_PIXELFORMAT DDSPF_A8B8G8R8{.size = sizeof(DDS_PIXELFORMAT),
                                         .flags = DDPF_RGBA,
                                         .fourCC = 0u,
                                         .rgbBitCount = 32u,
                                         .rBitMask = 0x000000ff,
                                         .gBitMask = 0x0000ff00,
                                         .bBitMask = 0x00ff0000,
                                         .aBitMask = 0xff000000};
constexpr DDS_PIXELFORMAT DDSPF_X8B8G8R8{.size = sizeof(DDS_PIXELFORMAT),
                                         .flags = DDPF_RGB,
                                         .fourCC = 0u,
                                         .rgbBitCount = 32u,
                                         .rBitMask = 0x000000ff,
                                         .gBitMask = 0x0000ff00,
                                         .bBitMask = 0x00ff0000,
                                         .aBitMask = 0x00000000};
constexpr DDS_PIXELFORMAT DDSPF_G16R16{.size = sizeof(DDS_PIXELFORMAT),
                                       .flags = DDPF_RGB,
                                       .fourCC = 0u,
                                       .rgbBitCount = 32u,
                                       .rBitMask = 0x0000ffff,
                                       .gBitMask = 0xffff0000,
                                       .bBitMask = 0x00000000,
                                       .aBitMask = 0x00000000};
constexpr DDS_PIXELFORMAT DDSPF_R5G6B5{.size = sizeof(DDS_PIXELFORMAT),
                                       .flags = DDPF_RGB,
                                       .fourCC = 0u,
                                       .rgbBitCount = 16u,
                                       .rBitMask = 0x0000f800,
                                       .gBitMask = 0x000007e0,
                                       .bBitMask = 0x0000001f,
                                       .aBitMask = 0x00000000};
constexpr DDS_PIXELFORMAT DDSPF_A1R5G5B5{.size = sizeof(DDS_PIXELFORMAT),
                                         .flags = DDPF_RGBA,
                                         .fourCC = 0u,
                                         .rgbBitCount = 16u,
                                         .rBitMask = 0x00007c00,
                                         .gBitMask = 0x000003e0,
                                         .bBitMask = 0x0000001f,
                                         .aBitMask = 0x00008000};
constexpr DDS_PIXELFORMAT DDSPF_A4R4G4B4{.size = sizeof(DDS_PIXELFORMAT),
                                         .flags = DDPF_RGBA,
                                         .fourCC = 0u,
                                         .rgbBitCount = 16u,
                                         .rBitMask = 0x00000f00,
                                         .gBitMask = 0x000000f0,
                                         .bBitMask = 0x0000000f,
                                         .aBitMask = 0x0000f000};
constexpr DDS_PIXELFORMAT DDSPF_R8G8B8{.size = sizeof(DDS_PIXELFORMAT),
                                       .flags = DDPF_RGB,
                                       .fourCC = 0u,
                                       .rgbBitCount = 24u,
                                       .rBitMask = 0x00ff0000,
                                       .gBitMask = 0x0000ff00,
                                       .bBitMask = 0x000000ff,
                                       .aBitMask = 0x00000000};
constexpr DDS_PIXELFORMAT DDSPF_L8{.size = sizeof(DDS_PIXELFORMAT),
                                   .flags = DDPF_LUMINANCE,
                                   .fourCC = 0u,
                                   .rgbBitCount = 8u,
                                   .rBitMask = 0xff,
                                   .gBitMask = 0x00,
                                   .bBitMask = 0x00,
                                   .aBitMask = 0x00};
constexpr DDS_PIXELFORMAT DDSPF_L16{.size = sizeof(DDS_PIXELFORMAT),
                                    .flags = DDPF_LUMINANCE,
                                    .fourCC = 0u,
                                    .rgbBitCount = 16u,
                                    .rBitMask = 0xffff,
                                    .gBitMask = 0x0000,
                                    .bBitMask = 0x0000,
                                    .aBitMask = 0x0000};
constexpr DDS_PIXELFORMAT DDSPF_A8L8{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_LUMINANCE_ALPHA,
                                     .fourCC = 0u,
                                     .rgbBitCount = 16u,
                                     .rBitMask = 0x00ff,
                                     .gBitMask = 0x0000,
                                     .bBitMask = 0x0000,
                                     .aBitMask = 0xff00};
constexpr DDS_PIXELFORMAT DDSPF_A8L8_ALT{.size = sizeof(DDS_PIXELFORMAT),
                                         .flags = DDPF_LUMINANCE_ALPHA,
                                         .fourCC = 0u,
                                         .rgbBitCount = 8u,
                                         .rBitMask = 0x00ff,
                                         .gBitMask = 0x0000,
                                         .bBitMask = 0x0000,
                                         .aBitMask = 0xff00};
constexpr DDS_PIXELFORMAT DDSPF_A8{.size = sizeof(DDS_PIXELFORMAT),
                                   .flags = DDPF_ALPHA,
                                   .fourCC = 0u,
                                   .rgbBitCount = 8u,
                                   .rBitMask = 0x00,
                                   .gBitMask = 0x00,
                                   .bBitMask = 0x00,
                                   .aBitMask = 0xff};
constexpr DDS_PIXELFORMAT DDSPF_V8U8{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_BUMPDUDV,
                                     .fourCC = 0u,
                                     .rgbBitCount = 16u,
                                     .rBitMask = 0x00ff,
                                     .gBitMask = 0xff00,
                                     .bBitMask = 0x0000,
                                     .aBitMask = 0x0000};
constexpr DDS_PIXELFORMAT DDSPF_Q8W8V8U8{.size = sizeof(DDS_PIXELFORMAT),
                                         .flags = DDPF_BUMPDUDV,
                                         .fourCC = 0u,
                                         .rgbBitCount = 32u,
                                         .rBitMask = 0x000000ff,
                                         .gBitMask = 0x0000ff00,
                                         .bBitMask = 0x00ff0000,
                                         .aBitMask = 0xff000000};
constexpr DDS_PIXELFORMAT DDSPF_V16U16{.size = sizeof(DDS_PIXELFORMAT),
                                       .flags = DDPF_BUMPDUDV,
                                       .fourCC = 0u,
                                       .rgbBitCount = 32u,
                                       .rBitMask = 0x0000ffff,
                                       .gBitMask = 0xffff0000,
                                       .bBitMask = 0x00000000,
                                       .aBitMask = 0x00000000};

constexpr DDS_PIXELFORMAT DDSPF_DX10{.size = sizeof(DDS_PIXELFORMAT),
                                     .flags = DDPF_FOURCC,
                                     .fourCC = makeFourCC('D', 'X', '1', '0')};

// All formats are listed from left to right, most-significant bit to least-significant bit. For example, D3DFORMAT_ARGB
// is ordered from the most-significant bit channel A(alpha), to the least-significant bit channel B(blue). When
// traversing surface data, the data is stored in memory from least-significant bit to most-significant bit, which means
// that the channel order in memory is from least-significant bit(blue) to most-significant bit(alpha).
//
// The default value for formats that contain undefined channels(G16R16, A8, andso on) is 1. The only exception is the
// A8 format, which is initialized to 000 for the three color channels.
//
// The order of the bits is from the most significant byte first, so D3DFMT_A8L8 indicates that the high byte of this
// 2-byte format is alpha.

typedef enum _D3DFORMAT
{
    D3DFMT_UNKNOWN = 0,

    D3DFMT_R8G8B8 = 20,
    D3DFMT_A8R8G8B8 = 21,
    D3DFMT_X8R8G8B8 = 22,
    D3DFMT_R5G6B5 = 23,
    D3DFMT_X1R5G5B5 = 24,
    D3DFMT_A1R5G5B5 = 25,
    D3DFMT_A4R4G4B4 = 26,
    D3DFMT_R3G3B2 = 27,
    D3DFMT_A8 = 28,
    D3DFMT_A8R3G3B2 = 29,
    D3DFMT_X4R4G4B4 = 30,
    D3DFMT_A2B10G10R10 = 31,
    D3DFMT_A8B8G8R8 = 32,
    D3DFMT_X8B8G8R8 = 33,
    D3DFMT_G16R16 = 34,
    D3DFMT_A2R10G10B10 = 35,
    D3DFMT_A16B16G16R16 = 36,

    D3DFMT_A8P8 = 40,
    D3DFMT_P8 = 41,

    D3DFMT_L8 = 50,
    D3DFMT_A8L8 = 51,
    D3DFMT_A4L4 = 52,

    D3DFMT_V8U8 = 60,
    D3DFMT_L6V5U5 = 61,
    D3DFMT_X8L8V8U8 = 62,
    D3DFMT_Q8W8V8U8 = 63,
    D3DFMT_V16U16 = 64,
    D3DFMT_A2W10V10U10 = 67,

    D3DFMT_D16_LOCKABLE = 70,
    D3DFMT_D32 = 71,
    D3DFMT_D15S1 = 73,
    D3DFMT_D24S8 = 75,
    D3DFMT_D24X8 = 77,
    D3DFMT_D24X4S4 = 79,
    D3DFMT_D16 = 80,

    D3DFMT_D32F_LOCKABLE = 82,
    D3DFMT_D24FS8 = 83,

#if !defined(D3D_DISABLE_9EX)
    D3DFMT_D32_LOCKABLE = 84,
    D3DFMT_S8_LOCKABLE = 85,
#endif // !D3D_DISABLE_9EX

    D3DFMT_L16 = 81,

    D3DFMT_VERTEXDATA = 100,
    D3DFMT_INDEX16 = 101,
    D3DFMT_INDEX32 = 102,

    D3DFMT_Q16W16V16U16 = 110,

    D3DFMT_R16F = 111,
    D3DFMT_G16R16F = 112,
    D3DFMT_A16B16G16R16F = 113,

    D3DFMT_R32F = 114,
    D3DFMT_G32R32F = 115,
    D3DFMT_A32B32G32R32F = 116,

    D3DFMT_CxV8U8 = 117,

#if !defined(D3D_DISABLE_9EX)
    D3DFMT_A1 = 118,
    D3DFMT_A2B10G10R10_XR_BIAS = 119,
    D3DFMT_BINARYBUFFER = 199,
#endif // !D3D_DISABLE_9EX

    D3DFMT_FORCE_DWORD = 0x7fffffff
} D3DFORMAT;

constexpr const char* to_string(dds::D3DFORMAT format)
{
    switch(format)
    {
    case dds::D3DFMT_UNKNOWN: return "D3DFMT_UNKNOWN";
    case dds::D3DFMT_R8G8B8: return "D3DFMT_R8G8B8";
    case dds::D3DFMT_A8R8G8B8: return "D3DFMT_A8R8G8B8";
    case dds::D3DFMT_X8R8G8B8: return "D3DFMT_X8R8G8B8";
    case dds::D3DFMT_R5G6B5: return "D3DFMT_R5G6B5";
    case dds::D3DFMT_X1R5G5B5: return "D3DFMT_X1R5G5B5";
    case dds::D3DFMT_A1R5G5B5: return "D3DFMT_A1R5G5B5";
    case dds::D3DFMT_A4R4G4B4: return "D3DFMT_A4R4G4B4";
    case dds::D3DFMT_R3G3B2: return "D3DFMT_R3G3B2";
    case dds::D3DFMT_A8: return "D3DFMT_A8";
    case dds::D3DFMT_A8R3G3B2: return "D3DFMT_A8R3G3B2";
    case dds::D3DFMT_X4R4G4B4: return "D3DFMT_X4R4G4B4";
    case dds::D3DFMT_A2B10G10R10: return "D3DFMT_A2B10G10R10";
    case dds::D3DFMT_A8B8G8R8: return "D3DFMT_A8B8G8R8";
    case dds::D3DFMT_X8B8G8R8: return "D3DFMT_X8B8G8R8";
    case dds::D3DFMT_G16R16: return "D3DFMT_G16R16";
    case dds::D3DFMT_A2R10G10B10: return "D3DFMT_A2R10G10B10";
    case dds::D3DFMT_A16B16G16R16: return "D3DFMT_A16B16G16R16";
    case dds::D3DFMT_A8P8: return "D3DFMT_A8P8";
    case dds::D3DFMT_P8: return "D3DFMT_P8";
    case dds::D3DFMT_L8: return "D3DFMT_L8";
    case dds::D3DFMT_A8L8: return "D3DFMT_A8L8";
    case dds::D3DFMT_A4L4: return "D3DFMT_A4L4";
    case dds::D3DFMT_V8U8: return "D3DFMT_V8U8";
    case dds::D3DFMT_L6V5U5: return "D3DFMT_L6V5U5";
    case dds::D3DFMT_X8L8V8U8: return "D3DFMT_X8L8V8U8";
    case dds::D3DFMT_Q8W8V8U8: return "D3DFMT_Q8W8V8U8";
    case dds::D3DFMT_V16U16: return "D3DFMT_V16U16";
    case dds::D3DFMT_A2W10V10U10: return "D3DFMT_A2W10V10U10";
    case dds::D3DFMT_D16_LOCKABLE: return "D3DFMT_D16_LOCKABLE";
    case dds::D3DFMT_D32: return "D3DFMT_D32";
    case dds::D3DFMT_D15S1: return "D3DFMT_D15S1";
    case dds::D3DFMT_D24S8: return "D3DFMT_D24S8";
    case dds::D3DFMT_D24X8: return "D3DFMT_D24X8";
    case dds::D3DFMT_D24X4S4: return "D3DFMT_D24X4S4";
    case dds::D3DFMT_D16: return "D3DFMT_D16";
    case dds::D3DFMT_D32F_LOCKABLE: return "D3DFMT_D32F_LOCKABLE";
    case dds::D3DFMT_D24FS8: return "D3DFMT_D24FS8";
    case dds::D3DFMT_L16: return "D3DFMT_L16";
    case dds::D3DFMT_VERTEXDATA: return "D3DFMT_VERTEXDATA";
    case dds::D3DFMT_INDEX16: return "D3DFMT_INDEX16";
    case dds::D3DFMT_INDEX32: return "D3DFMT_INDEX32";
    case dds::D3DFMT_Q16W16V16U16: return "D3DFMT_Q16W16V16U16";
    case dds::D3DFMT_R16F: return "D3DFMT_R16F";
    case dds::D3DFMT_G16R16F: return "D3DFMT_G16R16F";
    case dds::D3DFMT_A16B16G16R16F: return "D3DFMT_A16B16G16R16F";
    case dds::D3DFMT_R32F: return "D3DFMT_R32F";
    case dds::D3DFMT_G32R32F: return "D3DFMT_G32R32F";
    case dds::D3DFMT_A32B32G32R32F: return "D3DFMT_A32B32G32R32F";
    case dds::D3DFMT_CxV8U8: return "D3DFMT_CxV8U8";
    case dds::D3DFMT_A1: return "D3DFMT_A1";
    case dds::D3DFMT_A2B10G10R10_XR_BIAS: return "D3DFMT_A2B10G10R10_XR_BIAS";
    case dds::D3DFMT_BINARYBUFFER: return "D3DFMT_BINARYBUFFER";
    case dds::D3DFMT_FORCE_DWORD: return "D3DFMT_FORCE_DWORD";
    default: return "";
    }
}

[[nodiscard]] constexpr gpufmt::Format fourCCToFormat(uint32_t fourCC) noexcept
{
    switch(fourCC)
    {
    case makeFourCC('D', 'X', 'T', '1'): return gpufmt::Format::BC1_RGBA_UNORM_BLOCK;
    case makeFourCC('D', 'X', 'T', '2'): return gpufmt::Format::BC2_UNORM_BLOCK;
    case makeFourCC('D', 'X', 'T', '3'): return gpufmt::Format::BC2_UNORM_BLOCK;
    case makeFourCC('D', 'X', 'T', '4'): return gpufmt::Format::BC3_UNORM_BLOCK;
    case makeFourCC('D', 'X', 'T', '5'): return gpufmt::Format::BC3_UNORM_BLOCK;
    case makeFourCC('B', 'C', '4', 'U'): return gpufmt::Format::BC4_UNORM_BLOCK;
    case makeFourCC('B', 'C', '4', 'S'): return gpufmt::Format::BC4_SNORM_BLOCK;
    case makeFourCC('B', 'C', '5', 'U'): return gpufmt::Format::BC5_UNORM_BLOCK;
    case makeFourCC('B', 'C', '5', 'S'): return gpufmt::Format::BC5_SNORM_BLOCK;
    case makeFourCC('R', 'G', 'B', 'G'): return gpufmt::Format::UNDEFINED;
    case makeFourCC('G', 'R', 'G', 'B'): return gpufmt::Format::UNDEFINED;
    case makeFourCC('Y', 'U', 'Y', '2'): return gpufmt::Format::UNDEFINED;
    case makeFourCC('A', 'T', 'I', '1'): return gpufmt::Format::BC4_UNORM_BLOCK;
    case makeFourCC('A', 'T', 'I', '2'): return gpufmt::Format::BC5_UNORM_BLOCK;
    case makeFourCC('B', 'C', '6', 'H'): return gpufmt::Format::BC6H_UFLOAT_BLOCK;
    case makeFourCC('B', 'C', '7', 'L'): return gpufmt::Format::BC7_UNORM_BLOCK;
    case makeFourCC('E', 'A', 'R', ' '): return gpufmt::Format::EAC_R11_UNORM_BLOCK;
    case makeFourCC('E', 'A', 'R', 'G'): return gpufmt::Format::EAC_R11G11_UNORM_BLOCK;
    case makeFourCC('E', 'T', '2', ' '): return gpufmt::Format::ETC2_R8G8B8_UNORM_BLOCK;
    case makeFourCC('E', 'T', '2', 'A'): return gpufmt::Format::ETC2_R8G8B8A8_UNORM_BLOCK;
    case D3DFMT_R8G8B8: return gpufmt::Format::B8G8R8_UNORM;
    case D3DFMT_A8R8G8B8: return gpufmt::Format::B8G8R8A8_UNORM;
    case D3DFMT_X8R8G8B8: return gpufmt::Format::B8G8R8X8_UNORM;
    case D3DFMT_R5G6B5: return gpufmt::Format::R5G6B5_UNORM_PACK16;
    case D3DFMT_X1R5G5B5: return gpufmt::Format::UNDEFINED;
    case D3DFMT_A1R5G5B5: return gpufmt::Format::A1R5G5B5_UNORM_PACK16;
    case D3DFMT_A4R4G4B4: return gpufmt::Format::A4R4G4B4_UNORM_PACK16;
    case D3DFMT_R3G3B2: return gpufmt::Format::UNDEFINED;
    case D3DFMT_A8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_A8R3G3B2: return gpufmt::Format::UNDEFINED;
    case D3DFMT_X4R4G4B4: return gpufmt::Format::UNDEFINED;
    case D3DFMT_A2B10G10R10: return gpufmt::Format::A2B10G10R10_UNORM_PACK32;
    case D3DFMT_A8B8G8R8: return gpufmt::Format::R8G8B8A8_UNORM;
    case D3DFMT_X8B8G8R8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_G16R16: return gpufmt::Format::R16G16_UNORM;
    case D3DFMT_A2R10G10B10: return gpufmt::Format::A2R10G10B10_UNORM_PACK32;
    case D3DFMT_A16B16G16R16: return gpufmt::Format::R16G16B16A16_UNORM;
    case D3DFMT_A8P8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_P8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_L8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_A8L8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_A4L4: return gpufmt::Format::UNDEFINED;
    case D3DFMT_V8U8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_L6V5U5: return gpufmt::Format::UNDEFINED;
    case D3DFMT_X8L8V8U8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_Q8W8V8U8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_V16U16: return gpufmt::Format::UNDEFINED;
    case D3DFMT_A2W10V10U10: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D16_LOCKABLE: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D32: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D15S1: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D24S8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D24X8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D24X4S4: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D16: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D32F_LOCKABLE: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D24FS8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_D32_LOCKABLE: return gpufmt::Format::UNDEFINED;
    case D3DFMT_S8_LOCKABLE: return gpufmt::Format::UNDEFINED;
    case D3DFMT_L16: return gpufmt::Format::UNDEFINED;
    case D3DFMT_VERTEXDATA: return gpufmt::Format::UNDEFINED;
    case D3DFMT_INDEX16: return gpufmt::Format::UNDEFINED;
    case D3DFMT_INDEX32: return gpufmt::Format::UNDEFINED;
    case D3DFMT_Q16W16V16U16: return gpufmt::Format::UNDEFINED;
    case D3DFMT_R16F: return gpufmt::Format::R16_SFLOAT;
    case D3DFMT_G16R16F: return gpufmt::Format::R16G16_SFLOAT;
    case D3DFMT_A16B16G16R16F: return gpufmt::Format::R16G16B16A16_SFLOAT;
    case D3DFMT_R32F: return gpufmt::Format::R32_SFLOAT;
    case D3DFMT_G32R32F: return gpufmt::Format::R32G32_SFLOAT;
    case D3DFMT_A32B32G32R32F: return gpufmt::Format::R32G32B32A32_SFLOAT;
    case D3DFMT_CxV8U8: return gpufmt::Format::UNDEFINED;
    case D3DFMT_A1: return gpufmt::Format::UNDEFINED;
    case D3DFMT_A2B10G10R10_XR_BIAS: return gpufmt::Format::UNDEFINED;
    case D3DFMT_BINARYBUFFER: return gpufmt::Format::UNDEFINED;
    default: return gpufmt::Format::UNDEFINED;
    }
}
} // namespace teximp::dds