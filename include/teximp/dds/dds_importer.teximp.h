#pragma once

#include <teximp/teximp.h>

#ifdef TEXIMP_ENABLE_DDS_BACKEND_TEXIMP

#include <teximp/dds/dds.h>

namespace teximp::dds
{
class DdsTexImpImporter final : public TextureImporter
{
public:
    FileFormat fileFormat() const final;

    const dds::DDS_HEADER& header() const;
    const dds::DDS_HEADER_DXT10& header10() const;

protected:
    bool checkSignature(std::istream& stream) final;
    void load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options) final;

    dds::DDS_HEADER mHeader;
    dds::DDS_HEADER_DXT10 mHeader10;
};
} // namespace teximp::dds

#endif // TEXIMP_ENABLE_DDS_BACKEND_TEXIMP