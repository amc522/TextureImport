#pragma once

#include <teximp/config.h>

#ifdef TEXIMP_ENABLE_BITMAP_BACKEND_WIC

#include <teximp/teximp.h>

namespace teximp::bitmap
{
class BitmapWicImporter : public TextureImporter
{
public:
    virtual FileFormat fileFormat() const final;

protected:
    virtual bool checkSignature(std::istream& stream) final;
    virtual void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) final;
};
} // namespace teximp::bitmap

#endif // TEXIMP_ENABLE_BITMAP_BACKEND_WIC