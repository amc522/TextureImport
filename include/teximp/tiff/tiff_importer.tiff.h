#pragma once

#include <teximp/teximp.h>

#ifdef TEXIMP_ENABLE_TIFF_BACKEND_TIFF

struct tiff;

namespace teximp::tiff
{
class TiffTexImpImporter final : public TextureImporter
{
public:
    virtual FileFormat fileFormat() const final;

    void setErrorMessage(std::string&& errorMessage) { mErrorMessage = std::move(errorMessage); }

protected:
    virtual bool checkSignature(std::istream& stream) final;
    virtual void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) final;

private:
    struct TiffHeader
    {
        uint16_t identifier;
        uint16_t version;
        uint32_t ifdOffset;
    };

    ::tiff* mTiff = nullptr;
};
} // namespace teximp::tiff

#endif // TEXIMP_ENABLE_PNG_BACKEND_LIBPNG