#include "texture_importer_factory.h"

#include <cputex/converter.h>
#include <cputex/string.h>
#include <gpufmt/string.h>
#include <teximp/string.h>
#include <teximp/teximp.h>

#include <algorithm>
#include <format>
#include <fstream>
#include <locale>

namespace teximp
{
class NullTextureImporter : public TextureImporter
{
public:
    NullTextureImporter(TextureImportError error) { mError = error; }

    NullTextureImporter(TextureImportError error, std::string errorMessage)
    {
        mError = error;
        mErrorMessage = std::move(errorMessage);
    }

    void setFilePath(std::filesystem::path filePath) { mFilePath = std::move(filePath); }

    virtual FileFormat fileFormat() const { return FileFormat::Undefined; }

protected:
    virtual bool checkSignature(std::istream& /*stream*/) { return false; }
    virtual void load(std::istream& /*stream*/, ITextureAllocator& /*textureAllocator*/,
                      TextureImportOptions /*options*/)
    {}
};

std::span<FileFormat> supportedFileFormats() noexcept
{
    static std::array<FileFormat, (size_t)FileFormat::Count> fileFormats;
    static bool initialized = false;

    if(initialized) { return fileFormats; }

    for(size_t i = 0; i < (size_t)FileFormat::Count; ++i)
    {
        fileFormats[i] = (FileFormat)i;
    }

    initialized = true;

    return fileFormats;
}

std::span<const std::span<const std::string_view>> supportedFileFormatExtensions()
{
    static std::array<std::span<const std::string_view>, (size_t)FileFormat::Count> imageContainerExtensions;

    static bool initialized = false;

    if(initialized) { return imageContainerExtensions; }

#ifdef TEXIMP_ENABLE_BITMAP
    imageContainerExtensions[(size_t)FileFormat::Bitmap] = kBitmapExtensions;
#endif
#ifdef TEXIMP_ENABLE_DDS
    imageContainerExtensions[(size_t)FileFormat::Dds] = kDdsExtensions;
#endif
#ifdef TEXIMP_ENABLE_EXR
    imageContainerExtensions[(size_t)FileFormat::Exr] = kExrExtensions;
#endif
#ifdef TEXIMP_ENABLE_JPEG
    imageContainerExtensions[(size_t)FileFormat::Jpeg] = kJpegExtensions;
#endif
#ifdef TEXIMP_ENABLE_KTX
    imageContainerExtensions[(size_t)FileFormat::Ktx] = kKtxExtensions;
#endif
#ifdef TEXIMP_ENABLE_PNG
    imageContainerExtensions[(size_t)FileFormat::Png] = kPngExtensions;
#endif
#ifdef TEXIMP_ENABLE_TARGA
    imageContainerExtensions[(size_t)FileFormat::Targa] = kTargaExtensions;
#endif
#ifdef TEXIMP_ENABLE_TIFF
    imageContainerExtensions[(size_t)FileFormat::Tiff] = kTiffExtensions;
#endif

    initialized = true;

    return imageContainerExtensions;
}

std::span<const std::string_view> fileFormatExtensions(FileFormat fileFormat)
{
    if(fileFormat < FileFormat::Count) { return supportedFileFormatExtensions()[(size_t)fileFormat]; }

    return {};
}

std::optional<FileFormat> getFileFormatFromExtension(std::string_view extension)
{
    if(extension.empty()) { return std::nullopt; }

    if(extension[0] == '.') { extension.remove_prefix(1); }

    if(extension.empty())
    {
        // the extension was only '.'
        return std::nullopt;
    }

    if(extension.length() > 64) { return std::nullopt; }

    std::array<char, 64> lowercaseExtension;
    lowercaseExtension.fill('\0');
    auto currentLocale = std::locale();
    std::transform(std::begin(extension), std::end(extension), std::begin(lowercaseExtension),
                   [&currentLocale](const char c) { return std::tolower(c, currentLocale); });

    std::string_view lowercaseExtensionSpan(lowercaseExtension.data(), extension.size());
    const std::span<const std::span<const std::string_view>>& formatExtensions = supportedFileFormatExtensions();

    for(size_t extensionTypeIndex = 0; extensionTypeIndex < formatExtensions.size(); ++extensionTypeIndex)
    {
        for(std::string_view currExtention : formatExtensions[extensionTypeIndex])
        {
            if(currExtention == lowercaseExtensionSpan) { return supportedFileFormats()[extensionTypeIndex]; }
        }
    }

    return std::nullopt;
}

TextureImportResult importTexture(const std::filesystem::path& filePath, TextureImportOptions options,
                                  PreferredBackends preferredBackends)
{
    DefaultTextureAllocator textureAllocator;
    std::unique_ptr<TextureImporter> importer = importTexture(filePath, textureAllocator, options, preferredBackends);

    return TextureImportResult{.importer = std::move(importer), .textureAllocator = std::move(textureAllocator)};
}

std::unique_ptr<TextureImporter> importTexture(const std::filesystem::path& filePath,
                                               ITextureAllocator& textureAllocator, TextureImportOptions options,
                                               PreferredBackends preferredBackends)
{
    if(!std::filesystem::exists(filePath))
    {
        auto importer = std::make_unique<NullTextureImporter>(TextureImportError::FileNotFound);
        importer->setFilePath(filePath);
        return importer;
    }

    std::ifstream imageStream(filePath, std::ios::in | std::ios::binary);

    if(!imageStream)
    {
        auto importer = std::make_unique<NullTextureImporter>(TextureImportError::FailedToOpenFile);
        importer->setFilePath(filePath);
        return importer;
    }

    if(filePath.has_extension())
    {
        std::string extension = filePath.extension().string();
        std::string lowerCaseExtension;
        std::locale currentLocale;

        std::transform(std::begin(extension), std::end(extension), std::back_inserter(lowerCaseExtension),
                       [&currentLocale](const char character) { return std::tolower(character, currentLocale); });

        std::optional<FileFormat> fileFormatResult = getFileFormatFromExtension(lowerCaseExtension);

        if(fileFormatResult)
        {
            auto importer = TextureImporterFactory::makeTextureImporter(
                fileFormatResult.value(), textureAllocator, options, preferredBackends, filePath, imageStream);

            if(importer) { return importer; }

            imageStream.seekg(0);
        }
    }

    for(FileFormat fileFormat : supportedFileFormats())
    {
        auto importer = TextureImporterFactory::makeTextureImporter(fileFormat, textureAllocator, options,
                                                                    preferredBackends, filePath, imageStream);

        if(importer) { return importer; }

        imageStream.seekg(0);
    }

    auto importer = std::make_unique<NullTextureImporter>(TextureImportError::UnknownFileFormat);
    importer->setFilePath(filePath);
    return importer;
}

TextureImporter::TextureImporter(std::filesystem::path filePath)
    : mFilePath(std::move(filePath))
{}

const std::filesystem::path& TextureImporter::filePath() const
{
    return mFilePath;
}

TextureImportStatus TextureImporter::status() const
{
    return mStatus;
}

TextureImportError TextureImporter::error() const
{
    return mError;
}

const std::string& TextureImporter::errorMessage() const
{
    return mErrorMessage;
}

void TextureImporter::setError(TextureImportError error)
{
    mStatus = TextureImportStatus::Error;
    mError = error;
}

void TextureImporter::setError(TextureImportError error, const std::string& errorMessage)
{
    mStatus = TextureImportStatus::Error;
    mError = error;
    mErrorMessage = errorMessage;
}

void TextureImporter::setError(TextureImportError error, std::string&& errorMessage)
{
    mStatus = TextureImportStatus::Error;
    mError = error;
    mErrorMessage = std::move(errorMessage);
}

void TextureImporter::setTextureAllocationError(const TextureParams& textureParams)
{
    setError(TextureImportError::TextureAllocationFailed,
             std::format("Failed to allocate texture. Format: {}, dimension: {}, extent: ({}, {}, {}), arraySize: {}, "
                         "faces: {}, mips: {}",
                         gpufmt::toString(textureParams.format), cputex::toString(textureParams.dimension),
                         textureParams.extent.x, textureParams.extent.y, textureParams.extent.z,
                         textureParams.arraySize, textureParams.faces, textureParams.mips));
}

void TextureImporter::setTextureAllocatorFormatLayoutError(FormatLayout formatLayout)
{
    setError(TextureImportError::InvalidTextureAllocatorFormatLayout,
             std::format("Invalid format layout '{}' selected by the texture allocator.", toString(formatLayout)));
}

void TextureImporter::setTextureAllocatorFormatError(gpufmt::Format format)
{
    setError(TextureImportError::InvalidTextureAllocatorFormat,
             std::format("Invalid format '{}' selected by the texture allocator.", gpufmt::toString(format)));
}

void CpuTexTextureAllocator::preAllocation(std::optional<int> textureCount)
{
    if(textureCount) { mTextures.reserve(textureCount.value()); }
}

bool CpuTexTextureAllocator::allocateTexture(const TextureParams& textureParams, int /*textureIndex*/)
{
    mTextures.emplace_back(textureParams);
    return true;
}

void CpuTexTextureAllocator::postAllocation() {}

std::span<std::byte> CpuTexTextureAllocator::accessTextureData(int textureIndex, const MipSurfaceKey& key)
{
    return mTextures[textureIndex].accessMipSurfaceData(key.arraySlice, key.face, key.mip);
}
} // namespace teximp