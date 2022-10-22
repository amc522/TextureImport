#pragma once

#include <teximp/teximp.h>

#ifdef TEXIMP_ENABLE_KTX_BACKEND_TEXIMP

#include <teximp/ktx/ktx.h>

#include <vector>

namespace teximp::ktx
{
class KtxTexImpImporter : public TextureImporter
{
public:
    using KeyValuePair = std::pair<std::string_view, std::span<const uint8_t>>;

    std::span<const char> fileIdentifier() const;
    const ktx::header10& header() const;
    const std::vector<uint8_t>& keyValueData() const;
    const std::vector<KeyValuePair>& keyValuePairs() const;

protected:
    FileFormat fileFormat() const override;
    bool checkSignature(std::istream& stream) override;
    void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) override;

private:
    std::array<char, 12> mFileIdentifier;
    ktx::header10 mHeader;
    std::vector<uint8_t> mKeyValueData;
    std::vector<KeyValuePair> mKeyValuePairs;
};
} // namespace teximp::ktx

#endif // TEXIMP_ENABLE_KTX_BACKEND_TEXIMP