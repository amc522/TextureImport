#include "texture_importer_factory.h"

#include <teximp/teximp.h>

#include <teximp/bitmap/bitmap_importer.teximp.h>
#include <teximp/bitmap/bitmap_importer.wic.h>
#include <teximp/dds/dds_importer.teximp.h>
#include <teximp/exr/exr_importer.openexr.h>
#include <teximp/jpeg/jpeg_importer.libjpeg_turbo.h>
#include <teximp/ktx/ktx_importer.teximp.h>
#include <teximp/png/png_importer.libpng.h>
#include <teximp/targa/targa_importer.teximp.h>
#include <teximp/tiff/tiff_importer.tiff.h>

namespace teximp
{
#ifdef TEXIMP_ENABLE_BITMAP
[[nodiscard]] std::unique_ptr<TextureImporter> createBitmapLoader(BitmapImporterBackend backend)
{
    switch(backend)
    {
#ifdef TEXIMP_ENABLE_BITMAP_BACKEND_TEXIMP
    case BitmapImporterBackend::TexImp: return std::make_unique<bitmap::BitmapTexImpImporter>();
#endif

#ifdef TEXIMP_ENABLE_BITMAP_BACKEND_WIC
    case BitmapImporterBackend::Wic: return std::make_unique<bitmap::BitmapWicImporter>();
#endif

    default: return nullptr;
    }
}
#endif // TEXIMP_ENABLE_BITMAP

#ifdef TEXIMP_ENABLE_DDS
[[nodiscard]] std::unique_ptr<TextureImporter> createDdsLoader(DdsImporterBackend backend)
{
    switch(backend)
    {
#ifdef TEXIMP_ENABLE_DDS_BACKEND_TEXIMP
    case DdsImporterBackend::TexImp: return std::make_unique<dds::DdsTexImpImporter>();
#endif

    default: return nullptr;
    }
}
#endif // TEXIMP_ENABLE_DDS

#ifdef TEXIMP_ENABLE_EXR
[[nodiscard]] std::unique_ptr<TextureImporter> createExrLoader(ExrImporterBackend backend)
{
    switch(backend)
    {
#ifdef TEXIMP_ENABLE_EXR_BACKEND_OPENEXR
    case ExrImporterBackend::OpenExr: return std::make_unique<exr::ExrOpenExrImporter>();
#endif

    default: return nullptr;
    }
}
#endif // TEXIMP_ENABLE_EXR

#ifdef TEXIMP_ENABLE_JPEG
[[nodiscard]] std::unique_ptr<TextureImporter> createJpeegLoader(JpegImporterBackend backend)
{
    switch(backend)
    {
#ifdef TEXIMP_ENABLE_JPEG_BACKEND_LIBJPEG_TURBO
    case JpegImporterBackend::LibJpegTurbo: return std::make_unique<jpeg::JpegLibJpegTurboImporter>();
#endif

    default: return nullptr;
    }
}
#endif // TEXIMP_ENABLE_JPEG

#ifdef TEXIMP_ENABLE_KTX
[[nodiscard]] std::unique_ptr<TextureImporter> createKtxLoader(KtxImporterBackend backend)
{
    switch(backend)
    {
#ifdef TEXIMP_ENABLE_KTX_BACKEND_TEXIMP
    case KtxImporterBackend::TexImp: return std::make_unique<ktx::KtxTexImpImporter>();
#endif

    default: return nullptr;
    }
}
#endif // TEXIMP_ENABLE_KTX

#ifdef TEXIMP_ENABLE_PNG
[[nodiscard]] std::unique_ptr<TextureImporter> createPngLoader(PngImporterBackend backend)
{
    switch(backend)
    {
#ifdef TEXIMP_ENABLE_PNG_BACKEND_LIBPNG
    case PngImporterBackend::LibPng: return std::make_unique<png::PngLibPngImporter>();
#endif

    default: return nullptr;
    }
}
#endif // TEXIMP_ENABLE_PNG

#ifdef TEXIMP_ENABLE_TARGA
[[nodiscard]] std::unique_ptr<TextureImporter> createTargaLoader(TargaImporterBackend backend)
{
    switch(backend)
    {
#ifdef TEXIMP_ENABLE_TARGA_BACKEND_TEXIMP
    case TargaImporterBackend::TexImp: return std::make_unique<targa::TargaTexImpImporter>();
#endif

    default: return nullptr;
    }
}
#endif // TEXIMP_ENABLE_TARGA

#ifdef TEXIMP_ENABLE_TIFF
[[nodiscard]] std::unique_ptr<TextureImporter> createTiffLoader(TiffImporterBackend backend)
{
    switch(backend)
    {
#ifdef TEXIMP_ENABLE_TIFF_BACKEND_TIFF
    case TiffImporterBackend::Tiff: return std::make_unique<tiff::TiffTexImpImporter>();
#endif

    default: return nullptr;
    }
}
#endif // TEXIMP_ENABLE_TIFF

[[nodiscard]] std::unique_ptr<TextureImporter>
TextureImporterFactory::makeTextureImporter(FileFormat fileFormat, ITextureAllocator& textureAllocator,
                                            TextureImportOptions options, PreferredBackends preferredBackends,
                                            const std::filesystem::path& filePath, std::istream& stream)
{
    std::unique_ptr<TextureImporter> textureImporter;

    switch(fileFormat)
    {
#ifdef TEXIMP_ENABLE_BITMAP
    case FileFormat::Bitmap: textureImporter = createBitmapLoader(preferredBackends.bitmap); break;
#endif

#ifdef TEXIMP_ENABLE_DDS
    case FileFormat::Dds: textureImporter = createDdsLoader(preferredBackends.dds); break;
#endif

#ifdef TEXIMP_ENABLE_EXR
    case FileFormat::Exr: textureImporter = createExrLoader(preferredBackends.exr); break;
#endif

#ifdef TEXIMP_ENABLE_JPEG
    case FileFormat::Jpeg: textureImporter = createJpeegLoader(preferredBackends.jpeg); break;
#endif

#ifdef TEXIMP_ENABLE_KTX
    case FileFormat::Ktx: textureImporter = createKtxLoader(preferredBackends.ktx); break;
#endif

#ifdef TEXIMP_ENABLE_PNG
    case FileFormat::Png: textureImporter = createPngLoader(preferredBackends.png); break;
#endif

#ifdef TEXIMP_ENABLE_TARGA
    case FileFormat::Targa: textureImporter = createTargaLoader(preferredBackends.targa); break;
#endif

#ifdef TEXIMP_ENABLE_TIFF
    case FileFormat::Tiff: textureImporter = createTiffLoader(preferredBackends.tiff); break;
#endif

    default: break;
    }

    if(!textureImporter) { return nullptr; }

    if(!textureImporter->checkSignature(stream)) { return nullptr; }

    textureImporter->mFilePath = filePath;
    textureImporter->mTextureAllocator = &textureAllocator;
    
    try
    {
        textureImporter->load(stream, textureAllocator, options);
    }
    catch(const TextureImporterException&)
    {
    }

    return textureImporter;
}
} // namespace teximp