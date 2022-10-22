#pragma once

#include <filesystem>
#include <istream>
#include <memory>

namespace teximp
{
class ITextureAllocator;
class TextureImporter;
enum class FileFormat;
struct PreferredBackends;
struct TextureImportOptions;

class TextureImporterFactory
{
public:
    static [[nodiscard]] std::unique_ptr<TextureImporter>
    makeTextureImporter(FileFormat fileFormat, ITextureAllocator& textureAllocator, TextureImportOptions options,
                        PreferredBackends preferredBackends, const std::filesystem::path& filePath,
                        std::istream& stream);
};
} // namespace teximp