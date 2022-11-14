#pragma once

#include <cputex/unique_texture.h>
#include <teximp/config.h>
#include <tl/expected.hpp>

#include <array>
#include <filesystem>
#include <functional>
#include <istream>
#include <memory>
#include <span>
#include <string_view>

namespace cputex
{
enum class ConvertError;
} // namespace cputex

namespace teximp
{
enum class FileFormat
{
#ifdef TEXIMP_ENABLE_BITMAP
    Bitmap,
#endif
#ifdef TEXIMP_ENABLE_DDS
    Dds,
#endif
#ifdef TEXIMP_ENABLE_EXR
    Exr,
#endif
#ifdef TEXIMP_ENABLE_JPEG
    Jpeg,
#endif
#ifdef TEXIMP_ENABLE_KTX
    Ktx,
#endif
#ifdef TEXIMP_ENABLE_PNG
    Png,
#endif
#ifdef TEXIMP_ENABLE_TARGA
    Targa,
#endif
#ifdef TEXIMP_ENABLE_TIFF
    Tiff,
#endif
    Count,
    Undefined
};

#ifdef TEXIMP_ENABLE_BITMAP
enum class BitmapImporterBackend
{
#ifdef TEXIMP_ENABLE_BITMAP_BACKEND_TEXIMP
    TexImp,
#endif

#ifdef TEXIMP_ENABLE_BITMAP_BACKEND_WIC
    Wic,
#endif

    Default = 0
};
#endif

#ifdef TEXIMP_ENABLE_DDS
enum class DdsImporterBackend
{
#ifdef TEXIMP_ENABLE_DDS_BACKEND_TEXIMP
    TexImp,
#endif
    Default = 0
};
#endif

#ifdef TEXIMP_ENABLE_EXR
enum class ExrImporterBackend
{
#ifdef TEXIMP_ENABLE_EXR_BACKEND_OPENEXR
    OpenExr,
#endif
    Default = 0
};
#endif

#ifdef TEXIMP_ENABLE_JPEG
enum class JpegImporterBackend
{
#ifdef TEXIMP_ENABLE_JPEG_BACKEND_LIBJPEG_TURBO
    LibJpegTurbo,
#endif
    Default = 0
};
#endif

#ifdef TEXIMP_ENABLE_KTX
enum class KtxImporterBackend
{
#ifdef TEXIMP_ENABLE_KTX_BACKEND_TEXIMP
    TexImp,
#endif
    Default = 0
};
#endif

#ifdef TEXIMP_ENABLE_PNG
enum class PngImporterBackend
{
#ifdef TEXIMP_ENABLE_PNG_BACKEND_LIBPNG
    LibPng,
#endif
    Default = 0
};
#endif

#ifdef TEXIMP_ENABLE_TARGA
enum class TargaImporterBackend
{
#ifdef TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP
    TexImp,
#endif
    Default = 0
};
#endif
enum class TiffImporterBackend
{
#ifdef TEXIMP_ENABLE_TIFF_BACKEND_TIFF
    Tiff,
#endif
    Default = 0
};

struct PreferredBackends
{
#ifdef TEXIMP_ENABLE_BITMAP
    BitmapImporterBackend bitmap = BitmapImporterBackend::Default;
#endif
#ifdef TEXIMP_ENABLE_DDS
    DdsImporterBackend dds = DdsImporterBackend::Default;
#endif
#ifdef TEXIMP_ENABLE_EXR
    ExrImporterBackend exr = ExrImporterBackend::Default;
#endif
#ifdef TEXIMP_ENABLE_JPEG
    JpegImporterBackend jpeg = JpegImporterBackend::Default;
#endif
#ifdef TEXIMP_ENABLE_KTX
    KtxImporterBackend ktx = KtxImporterBackend::Default;
#endif
#ifdef TEXIMP_ENABLE_PNG
    PngImporterBackend png = PngImporterBackend::Default;
#endif
#ifdef TEXIMP_ENABLE_TARGA
    TargaImporterBackend targa = TargaImporterBackend::Default;
#endif
#ifdef TEXIMP_ENABLE_TIFF
    TiffImporterBackend tiff = TiffImporterBackend::Default;
#endif
};

constexpr std::array kBitmapExtensions = std::to_array<std::string_view>({"bmp"});
constexpr std::array kDdsExtensions = std::to_array<std::string_view>({"dds"});
constexpr std::array kExrExtensions = std::to_array<std::string_view>({"exr"});
constexpr std::array kJpegExtensions = std::to_array<std::string_view>({"jpg", "jpeg"});
constexpr std::array kKtxExtensions = std::to_array<std::string_view>({"ktx"});
constexpr std::array kPngExtensions = std::to_array<std::string_view>({"png"});
constexpr std::array kTargaExtensions = std::to_array<std::string_view>({"tga", "targa"});
constexpr std::array kTiffExtensions = std::to_array<std::string_view>({"tiff", "tif"});

constexpr int kMaxTextureWidth = 16384;
constexpr int kMaxTextureHeight = 16384;

struct TextureImportOptions
{
    bool padRgbWithAlpha = true;
    bool assumeSrgb = true;
};

enum class TextureImportStatus
{
    Loading,
    Success,
    Error
};

enum class TextureImportError
{
    None,
    FileNotFound,
    FailedToOpenFile,
    FailedToReadFile,
    SignatureNotRecognized,
    CouldNotReadHeader,
    NotEnoughData,
    InvalidDataInImage,
    DimensionsTooLarge,
    UnknownFormat,
    UnsupportedFeature,
    ConversionError,
    InvalidTextureAllocatorFormatLayout,
    InvalidTextureAllocatorFormat,
    TextureAllocationFailed,
    UnknownFileFormat,
    Unknown
};

enum class FormatLayout
{
    _4_4,
    _4_4_4_4,
    _5_6_5,
    _5_5_5_1,
    _8,
    _8_8,
    _8_8_8,
    _8_8_8_8,
    _9_9_9_5,
    _10_10_10_2,
    _11_11_10,
    _16,
    _16_16,
    _16_16_16,
    _16_16_16_16,
    _32,
    _32_32,
    _32_32_32,
    _32_32_32_32,
#ifndef GF_EXCLUDE_64BIT_FORMATS
    _64,
    _64_64,
    _64_64_64,
    _64_64_64_64,
#endif
};

[[nodiscard]] std::span<FileFormat> supportedFileFormats() noexcept;

[[nodiscard]] std::span<const std::span<const std::string_view>> supportedFileFormatExtensions();

[[nodiscard]] std::span<const std::string_view> fileFormatExtensions(FileFormat fileFormat);

[[nodiscard]] std::optional<FileFormat> getFileFormatFromExtension(std::string_view extension);

using TextureParams = cputex::TextureParams;

struct MipSurfaceKey
{
    int16_t arraySlice = 0;
    int8_t face = 0;
    int8_t mip = 0;
};

class ITextureAllocator
{
public:
    virtual ~ITextureAllocator() = default;

    virtual FormatLayout selectFormatLayout(FormatLayout nativeFormatLayout,
                                            std::span<const FormatLayout> /*additionalLayout*/)
    {
        return nativeFormatLayout;
    }
    virtual gpufmt::Format selectFormat(FormatLayout /*formatLayout*/, std::span<const gpufmt::Format> availableFormats)
    {
        return availableFormats.front();
    }

    virtual void preAllocation(std::optional<int> textureCount) = 0;
    virtual bool allocateTexture(const TextureParams& textureParams, int textureIndex) = 0;
    virtual void postAllocation() = 0;

    virtual std::span<std::byte> accessTextureData(int textureIndex, const MipSurfaceKey& key) = 0;
};

class CpuTexTextureAllocator : public ITextureAllocator
{
public:
    // Inherited via ITextureAllocator
    virtual void preAllocation(std::optional<int> textureCount) override;

    virtual bool allocateTexture(const TextureParams& textureParams, int textureIndex) override;

    virtual void postAllocation() override;

    virtual std::span<std::byte> accessTextureData(int textureIndex, const MipSurfaceKey& key) override;

    [[nodiscard]] std::span<const cputex::UniqueTexture> getTextures() const { return mTextures; }

    [[nodiscard]] std::span<cputex::UniqueTexture> getTextures() { return mTextures; }

private:
    std::vector<cputex::UniqueTexture> mTextures;
};

using DefaultTextureAllocator = CpuTexTextureAllocator;

class TextureImporter
{
public:
    TextureImporter(const TextureImporter&) = delete;
    TextureImporter(TextureImporter&&) = default;
    virtual ~TextureImporter() = default;

    TextureImporter& operator=(const TextureImporter&) = delete;
    TextureImporter& operator=(TextureImporter&&) = default;

    const std::filesystem::path& filePath() const;
    virtual FileFormat fileFormat() const = 0;
    TextureImportStatus status() const;
    TextureImportError error() const;
    const std::string& errorMessage() const;

    ITextureAllocator* textureAllocator() { return mTextureAllocator; }
    const ITextureAllocator* textureAllocator() const { return mTextureAllocator; }

protected:
    TextureImporter() = default;
    TextureImporter(std::filesystem::path filePath);

    virtual bool checkSignature(std::istream& stream) = 0;
    virtual void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) = 0;

    void setError(TextureImportError error);
    void setError(TextureImportError error, const std::string& errorMessage);
    void setError(TextureImportError error, std::string&& errorMessage);
    void setTextureAllocationError(const TextureParams& textureParams);
    void setTextureAllocatorFormatLayoutError(FormatLayout formatLayout);
    void setTextureAllocatorFormatError(gpufmt::Format format);

private:
    friend class TextureImporterFactory;

protected:
    std::filesystem::path mFilePath;

    TextureImportStatus mStatus = TextureImportStatus::Loading;
    TextureImportError mError = TextureImportError::None;
    std::string mErrorMessage;
    ITextureAllocator* mTextureAllocator = nullptr;
};

struct TextureImportResult
{
    std::unique_ptr<TextureImporter> importer;
    DefaultTextureAllocator textureAllocator;
};

[[nodiscard]] TextureImportResult importTexture(const std::filesystem::path& filePath,
                                                TextureImportOptions options = {},
                                                PreferredBackends preferredBackends = {});

std::unique_ptr<TextureImporter> importTexture(const std::filesystem::path& filePath,
                                               ITextureAllocator& textureAllocator, TextureImportOptions options = {},
                                               PreferredBackends preferredBackends = {});

template<class T>
[[nodiscard]] constexpr std::span<T> castWritableBytes(std::span<std::byte> bytes) noexcept
{
    return std::span<T>(reinterpret_cast<T*>(bytes.data()), bytes.size_bytes() / sizeof(T));
}
} // namespace teximp