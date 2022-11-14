#pragma once

#include <teximp/teximp.h>

#include <string_view>

namespace teximp
{
constexpr std::string_view toString(FileFormat fileFormat)
{
    switch(fileFormat)
    {
#ifdef TEXIMP_ENABLE_BITMAP
    case teximp::FileFormat::Bitmap:
        return "Bitmap";
#endif
#ifdef TEXIMP_ENABLE_DDS
    case teximp::FileFormat::Dds:
        return "Dds";
#endif
#ifdef TEXIMP_ENABLE_EXR
    case teximp::FileFormat::Exr:
        return "Exr";
#endif
#ifdef TEXIMP_ENABLE_JPEG
    case teximp::FileFormat::Jpeg:
        return "Jpeg";
#endif
#ifdef TEXIMP_ENABLE_KTX
    case teximp::FileFormat::Ktx:
        return "Ktx";
#endif
#ifdef TEXIMP_ENABLE_PNG
    case teximp::FileFormat::Png:
        return "Png";
#endif
#ifdef TEXIMP_ENABLE_TARGA
    case teximp::FileFormat::Targa:
        return "Targa";
#endif
#ifdef TEXIMP_ENABLE_TIFF
    case teximp::FileFormat::Tiff:
        return "Tiff";
#endif
    default:
        return "Unknown FileFormat";
    }
}

constexpr std::string_view toString(TextureImportError error)
{
    switch(error)
    {
    case TextureImportError::None:
        return "None";
    case TextureImportError::FileNotFound:
        return "FileNotFound";
    case TextureImportError::FailedToOpenFile:
        return "FailedToOpenFile";
    case TextureImportError::FailedToReadFile:
        return "FailedToReadFile";
    case TextureImportError::SignatureNotRecognized:
        return "SignatureNotRecognized";
    case TextureImportError::CouldNotReadHeader:
        return "CouldNotReadHeader";
    case TextureImportError::NotEnoughData:
        return "NotEnoughData";
    case TextureImportError::InvalidDataInImage:
        return "InvalidDataInImage";
    case TextureImportError::DimensionsTooLarge:
        return "DimensionsTooLarge";
    case TextureImportError::UnknownFormat:
        return "UnknownFormat";
    case TextureImportError::UnsupportedFeature:
        return "UnsupportedFeature";
    case TextureImportError::ConversionError:
        return "ConversionError";
    case TextureImportError::InvalidTextureAllocatorFormatLayout:
        return "InvalidTextureAllocatorFormatLayout";
    case TextureImportError::InvalidTextureAllocatorFormat:
        return "InvalidTextureAllocatorFormat";
    case TextureImportError::TextureAllocationFailed:
        return "TextureAllocationFailed";
    case TextureImportError::UnknownFileFormat:
        return "UnknownFileFormat";
    case TextureImportError::Unknown:
        return "Unknown";
    default:
        assert(false);
        return "Unknown TextureImportError";
    }
}

constexpr std::string_view toString(FormatLayout formatLayout)
{
    switch(formatLayout)
    {
    case teximp::FormatLayout::_4_4: return "4_4";
    case teximp::FormatLayout::_4_4_4_4: return "4_4_4_4";
    case teximp::FormatLayout::_5_6_5: return "5_6_5";
    case teximp::FormatLayout::_5_5_5_1: return "5_5_5_1";
    case teximp::FormatLayout::_8: return "8";
    case teximp::FormatLayout::_8_8: return "8_8";
    case teximp::FormatLayout::_8_8_8: return "8_8_8";
    case teximp::FormatLayout::_8_8_8_8: return "8_8_8_8";
    case teximp::FormatLayout::_10_10_10_2: return "10_10_10_2";
    case teximp::FormatLayout::_16: return "16";
    case teximp::FormatLayout::_16_16: return "16_16";
    case teximp::FormatLayout::_16_16_16: return "16_16_16";
    case teximp::FormatLayout::_16_16_16_16: return "16_16_16_16";
    case teximp::FormatLayout::_32: return "32";
    case teximp::FormatLayout::_32_32: return "32_32";
    case teximp::FormatLayout::_32_32_32: return "32_32_32";
    case teximp::FormatLayout::_32_32_32_32: return "32_32_32_32";
#ifndef GF_EXCLUDE_64BIT_FORMATS
    case teximp::FormatLayout::_64: return "64";
    case teximp::FormatLayout::_64_64: return "64_64";
    case teximp::FormatLayout::_64_64_64: return "64_64_64";
    case teximp::FormatLayout::_64_64_64_64: return "64_64_64_64";
#endif
    case teximp::FormatLayout::_11_11_10: return "11_11_10";
    case teximp::FormatLayout::_9_9_9_5: return "9_9_9_5";
    default: return "Unknown teximp::FormatLayout";
    }
}
} // namespace teximp