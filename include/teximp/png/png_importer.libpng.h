#pragma once

#include <teximp/teximp.h>

#ifdef TEXIMP_ENABLE_PNG_BACKEND_LIBPNG

namespace teximp::png
{
class PngLibPngImporter final : public TextureImporter
{
public:
    PngLibPngImporter() = default;
    PngLibPngImporter(const PngLibPngImporter&) = delete;
    PngLibPngImporter(PngLibPngImporter&&) = default;
    virtual ~PngLibPngImporter() = default;

    PngLibPngImporter& operator=(const PngLibPngImporter&) = delete;
    PngLibPngImporter& operator=(PngLibPngImporter&&) = default;

    FileFormat fileFormat() const final;

protected:
    bool checkSignature(std::istream& stream) final;
    void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) final;
};
} // namespace teximp::png

#endif // TEXIMP_ENABLE_PNG_BACKEND_LIBPNG