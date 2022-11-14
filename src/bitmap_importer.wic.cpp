#include <teximp/bitmap/bitmap_importer.wic.h>

#ifdef TEXIMP_ENABLE_BITMAP_BACKEND_WIC

#include "wic_manager.h"

#include <gpufmt/string.h>
#include <wincodec.h>

#include <atomic>

namespace teximp::bitmap
{
FileFormat BitmapWicImporter::fileFormat() const
{
    return FileFormat::Bitmap;
}

bool BitmapWicImporter::checkSignature(std::istream& stream)
{
    return WicManager::checkImageSignature(GUID_ContainerFormatBmp, stream);
}

void BitmapWicImporter::load(std::istream& stream, ITextureAllocator& textureAllocator, TextureImportOptions options)
{
    IWICImagingFactory* factory = WicManager::getFactory();

    if(factory == nullptr) { return; }

    // Create a decoder
    Microsoft::WRL::ComPtr<IWICBitmapDecoder> pDecoder;
    StdIStream* winStream = new StdIStream(stream);
    HRESULT hr = factory->CreateDecoderFromStream(winStream, nullptr, WICDecodeOptions::WICDecodeMetadataCacheOnDemand,
                                                  &pDecoder);

    Microsoft::WRL::ComPtr<IWICBitmapFrameDecode> frame;
    hr = pDecoder->GetFrame(0, &frame);

    if(FAILED(hr)) { return; }

    UINT width, height;
    hr = frame->GetSize(&width, &height);

    if(FAILED(hr)) { return; }

    WICPixelFormatGUID pixelFormat;
    hr = frame->GetPixelFormat(&pixelFormat);

    if(FAILED(hr)) { return; }

    BOOL paletteHasAlpha = false;

    if(WicManager::isPaletteFormat(pixelFormat))
    {
        Microsoft::WRL::ComPtr<IWICPalette> colorPalette;
        hr = factory->CreatePalette(&colorPalette);

        if(FAILED(hr)) { return; }

        colorPalette->HasAlpha(&paletteHasAlpha);
    }

    const auto primaryFormatLayoutResult = WicManager::wicPixelFormatToLayout(pixelFormat, paletteHasAlpha, options);

    if(!primaryFormatLayoutResult)
    {
        setError(primaryFormatLayoutResult.error(),
                 std::format("WICPixelFormat '{}' not supported.", WicManager::pixelFormatToString(pixelFormat)));
        return;
    }

    const auto additionalFormatLayouts = WicManager::getAllWiderFormatLayouts(primaryFormatLayoutResult.value());

    FormatLayout selectedFormatLayout;

    if(!additionalFormatLayouts.empty())
    {
        selectedFormatLayout =
            textureAllocator.selectFormatLayout(primaryFormatLayoutResult.value(), additionalFormatLayouts);

        if(selectedFormatLayout != primaryFormatLayoutResult.value() &&
           std::find(additionalFormatLayouts.begin(), additionalFormatLayouts.end(), selectedFormatLayout) !=
               additionalFormatLayouts.end())
        {
            setTextureAllocatorFormatLayoutError(selectedFormatLayout);
            return;
        }
    }
    else { selectedFormatLayout = primaryFormatLayoutResult.value(); }

    std::array<gpufmt::Format, 4> tempFormatsBuffer;
    std::span availableFormats =
        WicManager::getAvailableFormats(selectedFormatLayout, pixelFormat, paletteHasAlpha || options.padRgbWithAlpha,
                                        options.assumeSrgb, tempFormatsBuffer);

    gpufmt::Format selectedFormat;

    if(availableFormats.size() > 1)
    {
        selectedFormat = textureAllocator.selectFormat(selectedFormatLayout, availableFormats);
        if(std::find(availableFormats.begin(), availableFormats.end(), selectedFormat) == availableFormats.end())
        {
            setTextureAllocatorFormatError(selectedFormat);
            return;
        }
    }
    else if(availableFormats.empty())
    {
        setError(TextureImportError::Unknown);
        return;
    }
    else { selectedFormat = availableFormats.front(); }

    WICPixelFormatGUID outputWicPixelFormat = WicManager::gpuFormatToWICPixelFormat(selectedFormat);

    if(IsEqualGUID(outputWicPixelFormat, GUID_WICPixelFormatUndefined))
    {
        setError(TextureImportError::UnknownFormat,
                 std::format("Could not find valid WICPixelFormat for '{}'.", gpufmt::toString(selectedFormat)));
        return;
    }

    Microsoft::WRL::ComPtr<IWICFormatConverter> converter;
    hr = factory->CreateFormatConverter(&converter);

    if(FAILED(hr)) { return; }

    hr = converter->Initialize(frame.Get(), outputWicPixelFormat, WICBitmapDitherTypeNone, nullptr, 0.0,
                               WICBitmapPaletteTypeCustom);

    if(FAILED(hr)) { return; }

    cputex::TextureParams params;
    params.arraySize = 1;
    params.dimension = cputex::TextureDimension::Texture2D;
    params.extent = {width, height, 1};
    params.faces = 1;
    params.mips = 1;
    params.format = selectedFormat;

    textureAllocator.preAllocation(1);
    textureAllocator.allocateTexture(params, 0);
    textureAllocator.postAllocation();

    std::span<std::byte> byteData = textureAllocator.accessTextureData(0, {0, 0, 0});

    const gpufmt::FormatInfo& formatInfo = gpufmt::formatInfo(params.format);

    hr = converter->CopyPixels(nullptr, formatInfo.blockByteSize * width, (UINT)byteData.size_bytes(),
                               reinterpret_cast<BYTE*>(byteData.data()));

    if(FAILED(hr)) { return; }
}
} // namespace teximp::bitmap

#endif // TEXIMP_ENABLE_BITMAP_BACKEND_WIC