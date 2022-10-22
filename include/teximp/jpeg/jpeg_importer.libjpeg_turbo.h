#pragma once

#include <teximp/teximp.h>

#ifdef TEXIMP_ENABLE_JPEG_BACKEND_LIBJPEG_TURBO

#include <vector>

namespace teximp::jpeg
{
class JpegLibJpegTurboImporter final : public TextureImporter
{
public:
    virtual FileFormat fileFormat() const final;
    virtual bool checkSignature(std::istream& stream) final;
    virtual void load(std::istream& stream, ITextureAllocator& textureAlloctor, TextureImportOptions options) final;
};
} // namespace teximp::jpeg

#endif // TEXIMP_ENABLE_JPEG_BACKEND_LIBJPEG_TURBO