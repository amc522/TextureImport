#pragma once

#include <teximp/config.h>

#ifdef TEXIMP_PLATFORM_WINDOWS

#include <gpufmt/format.h>
#include <teximp/teximp.h>
#include <wincodec.h>
#include <wrl/client.h>

#include <array>
#include <istream>

namespace teximp
{
class StdIStream final : public IStream
{
public:
    StdIStream(std::istream& stream)
        : mStream(&stream)
    {
        std::streampos initialPos = mStream->tellg();
        mStream->seekg(0, std::ios::end);
        mStreamSize = mStream->tellg() - initialPos;

        mStream->seekg(0, std::ios::beg);
    }

    // Inherited via IStream
    STDMETHODIMP QueryInterface(REFIID riid, void** ppvObject) final
    {
        // The QISearch helper function requires including shlwapi.h and linking Shlwapi.lib. I do not think these extra
        // dependencies are worth it.
        //
        // static QITAB rgqit[] =
        // {
        //     QITABENT(StdIStream, ISequentialStream),
        //     QITABENT(StdIStream, IStream),
        //     { 0 },
        // };

        // return QISearch(this, rgqit, riid, ppvObject);

        if(!ppvObject) { return E_INVALIDARG; }

        if(riid == __uuidof(IUnknown) || riid == __uuidof(IStream) || riid == __uuidof(ISequentialStream))
        {
            *ppvObject = static_cast<IStream*>(this);
            this->AddRef();
            return S_OK;
        }

        return E_NOINTERFACE;
    }

    STDMETHODIMP_(ULONG) AddRef(void) override { return ++mRefCount; }

    STDMETHODIMP_(ULONG) Release(void) override
    {
        const auto newCount = --mRefCount;

        if(newCount == 0)
        {
            delete this;
            return 0;
        }

        return newCount;
    }

    STDMETHODIMP Read(void* pv, ULONG cb, ULONG* pcbRead) override
    {
        if(pv == nullptr || pcbRead == nullptr) { return STG_E_INVALIDPOINTER; }

        const std::streamsize bytesAvailable = mStreamSize - mStream->tellg();
        const std::streamsize bytesRead = std::min(static_cast<std::streamsize>(cb), bytesAvailable);
        mStream->read(static_cast<char*>(pv), bytesRead);
        *pcbRead = static_cast<ULONG>(bytesRead);

        return (*pcbRead == cb) ? S_OK : S_FALSE;
    }

    STDMETHODIMP Write(const void* /*pv*/, ULONG /*cb*/, ULONG* /*pcbWritten*/) override { return E_NOTIMPL; }

    STDMETHODIMP Seek(LARGE_INTEGER dlibMove, DWORD dwOrigin, ULARGE_INTEGER* plibNewPosition) override
    {
        switch(dwOrigin)
        {
        case STREAM_SEEK_CUR: mStream->seekg(static_cast<std::streamoff>(dlibMove.QuadPart), std::ios_base::cur); break;
        case STREAM_SEEK_END: mStream->seekg(static_cast<std::streamoff>(dlibMove.QuadPart), std::ios_base::end); break;
        case STREAM_SEEK_SET: mStream->seekg(static_cast<std::streamoff>(dlibMove.QuadPart), std::ios_base::beg); break;
        default: return STG_E_INVALIDFUNCTION;
        }

        if(plibNewPosition != nullptr) { plibNewPosition->QuadPart = mStream->tellg(); }

        return S_OK;
    }

    STDMETHODIMP SetSize(ULARGE_INTEGER /*libNewSize*/) override { return E_NOTIMPL; }

    STDMETHODIMP CopyTo(IStream* /*pstm*/, ULARGE_INTEGER /*cb*/, ULARGE_INTEGER* /*pcbRead*/,
                        ULARGE_INTEGER* /*pcbWritten*/) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP Commit(DWORD /*grfCommitFlags*/) override { return E_NOTIMPL; }

    STDMETHODIMP Revert(void) override { return E_NOTIMPL; }

    STDMETHODIMP LockRegion(ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/, DWORD /*dwLockType*/) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP UnlockRegion(ULARGE_INTEGER /*libOffset*/, ULARGE_INTEGER /*cb*/, DWORD /*dwLockType*/) override
    {
        return E_NOTIMPL;
    }

    STDMETHODIMP Stat(STATSTG* pstatstg, DWORD grfStatFlag) override
    {
        if(pstatstg == nullptr) { return STG_E_INVALIDPOINTER; }

        if(grfStatFlag == STATFLAG_DEFAULT) { return STG_E_INVALIDFLAG; }

        *pstatstg = {};
        pstatstg->type = STGTY_STREAM;
        pstatstg->cbSize.QuadPart = mStreamSize;
        pstatstg->grfMode = STGM_READ | STGM_WRITE;
        pstatstg->grfLocksSupported = LOCK_EXCLUSIVE;

        return S_OK;
    }

    STDMETHODIMP Clone(IStream** /*ppstm*/) override { return E_NOTIMPL; }

    std::atomic<ULONG> mRefCount = 0;
    std::istream* mStream = nullptr;
    std::streamoff mStreamSize = 0;
};

class WicManager
{
public:
    static IWICImagingFactory* getFactory()
    {
        if(mFactory != nullptr) { return mFactory.Get(); }

        // Initialize COM
        HRESULT hr = CoInitialize(nullptr);

        if(FAILED(hr)) { return nullptr; }

        // Create the COM imaging factory
        hr = CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&mFactory));

        if(FAILED(hr)) { return nullptr; }

        return mFactory.Get();
    }

    static std::string_view pixelFormatToString(WICPixelFormatGUID pixelFormat)
    {
        if(IsEqualGUID(pixelFormat, GUID_WICPixelFormatDontCare)) { return "WICPixelFormatDontCare"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat1bppIndexed)) { return "WICPixelFormat1bppIndexed"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat2bppIndexed)) { return "WICPixelFormat2bppIndexed"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat4bppIndexed)) { return "WICPixelFormat4bppIndexed"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppIndexed)) { return "WICPixelFormat8bppIndexed"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormatBlackWhite)) { return "WICPixelFormatBlackWhite"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat2bppGray)) { return "WICPixelFormat2bppGray"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat4bppGray)) { return "WICPixelFormat4bppGray"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppGray)) { return "WICPixelFormat8bppGray"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppAlpha)) { return "WICPixelFormat8bppAlpha"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppBGR555)) { return "WICPixelFormat16bppBGR555"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppBGR565)) { return "WICPixelFormat16bppBGR565"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppBGRA5551)) { return "WICPixelFormat16bppBGRA5551"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppGray)) { return "WICPixelFormat16bppGray"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat24bppBGR)) { return "WICPixelFormat24bppBGR"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat24bppRGB)) { return "WICPixelFormat24bppRGB"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppBGR)) { return "WICPixelFormat32bppBGR"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppBGRA)) { return "WICPixelFormat32bppBGRA"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppPBGRA)) { return "WICPixelFormat32bppPBGRA"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppGrayFloat)) { return "WICPixelFormat32bppGrayFloat"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppRGB)) { return "WICPixelFormat32bppRGB"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppRGBA)) { return "WICPixelFormat32bppRGBA"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppPRGBA)) { return "WICPixelFormat32bppPRGBA"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat48bppRGB)) { return "WICPixelFormat48bppRGB"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat48bppBGR)) { return "WICPixelFormat48bppBGR"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppRGB)) { return "WICPixelFormat64bppRGB"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppRGBA)) { return "WICPixelFormat64bppRGBA"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppBGRA)) { return "WICPixelFormat64bppBGRA"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppPRGBA)) { return "WICPixelFormat64bppPRGBA"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppPBGRA)) { return "WICPixelFormat64bppPBGRA"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppGrayFixedPoint))
        {
            return "WICPixelFormat16bppGrayFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppBGR101010)) { return "WICPixelFormat32bppBGR101010"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat48bppRGBFixedPoint))
        {
            return "WICPixelFormat48bppRGBFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat48bppBGRFixedPoint))
        {
            return "WICPixelFormat48bppBGRFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat96bppRGBFixedPoint))
        {
            return "WICPixelFormat96bppRGBFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat96bppRGBFloat)) { return "WICPixelFormat96bppRGBFloat"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat128bppRGBAFloat))
        {
            return "WICPixelFormat128bppRGBAFloat";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat128bppPRGBAFloat))
        {
            return "WICPixelFormat128bppPRGBAFloat";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat128bppRGBFloat)) { return "WICPixelFormat128bppRGBFloat"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppCMYK)) { return "WICPixelFormat32bppCMYK"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppRGBAFixedPoint))
        {
            return "WICPixelFormat64bppRGBAFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppBGRAFixedPoint))
        {
            return "WICPixelFormat64bppBGRAFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppRGBFixedPoint))
        {
            return "WICPixelFormat64bppRGBFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat128bppRGBAFixedPoint))
        {
            return "WICPixelFormat128bppRGBAFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat128bppRGBFixedPoint))
        {
            return "WICPixelFormat128bppRGBFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppRGBAHalf)) { return "WICPixelFormat64bppRGBAHalf"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppPRGBAHalf)) { return "WICPixelFormat64bppPRGBAHalf"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppRGBHalf)) { return "WICPixelFormat64bppRGBHalf"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat48bppRGBHalf)) { return "WICPixelFormat48bppRGBHalf"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppRGBE)) { return "WICPixelFormat32bppRGBE"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppGrayHalf)) { return "WICPixelFormat16bppGrayHalf"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppGrayFixedPoint))
        {
            return "WICPixelFormat32bppGrayFixedPoint";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppRGBA1010102))
        {
            return "WICPixelFormat32bppRGBA1010102";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppRGBA1010102XR))
        {
            return "WICPixelFormat32bppRGBA1010102XR";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppR10G10B10A2))
        {
            return "WICPixelFormat32bppR10G10B10A2";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppR10G10B10A2HDR10))
        {
            return "WICPixelFormat32bppR10G10B10A2HDR10";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bppCMYK)) { return "WICPixelFormat64bppCMYK"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat24bpp3Channels)) { return "WICPixelFormat24bpp3Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bpp4Channels)) { return "WICPixelFormat32bpp4Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat40bpp5Channels)) { return "WICPixelFormat40bpp5Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat48bpp6Channels)) { return "WICPixelFormat48bpp6Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat56bpp7Channels)) { return "WICPixelFormat56bpp7Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bpp8Channels)) { return "WICPixelFormat64bpp8Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat48bpp3Channels)) { return "WICPixelFormat48bpp3Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bpp4Channels)) { return "WICPixelFormat64bpp4Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat80bpp5Channels)) { return "WICPixelFormat80bpp5Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat96bpp6Channels)) { return "WICPixelFormat96bpp6Channels"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat112bpp7Channels))
        {
            return "WICPixelFormat112bpp7Channels";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat128bpp8Channels))
        {
            return "WICPixelFormat128bpp8Channels";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat40bppCMYKAlpha)) { return "WICPixelFormat40bppCMYKAlpha"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat80bppCMYKAlpha)) { return "WICPixelFormat80bppCMYKAlpha"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bpp3ChannelsAlpha))
        {
            return "WICPixelFormat32bpp3ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat40bpp4ChannelsAlpha))
        {
            return "WICPixelFormat40bpp4ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat48bpp5ChannelsAlpha))
        {
            return "WICPixelFormat48bpp5ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat56bpp6ChannelsAlpha))
        {
            return "WICPixelFormat56bpp6ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bpp7ChannelsAlpha))
        {
            return "WICPixelFormat64bpp7ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat72bpp8ChannelsAlpha))
        {
            return "WICPixelFormat72bpp8ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat64bpp3ChannelsAlpha))
        {
            return "WICPixelFormat64bpp3ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat80bpp4ChannelsAlpha))
        {
            return "WICPixelFormat80bpp4ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat96bpp5ChannelsAlpha))
        {
            return "WICPixelFormat96bpp5ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat112bpp6ChannelsAlpha))
        {
            return "WICPixelFormat112bpp6ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat128bpp7ChannelsAlpha))
        {
            return "WICPixelFormat128bpp7ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat144bpp8ChannelsAlpha))
        {
            return "WICPixelFormat144bpp8ChannelsAlpha";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppY)) { return "WICPixelFormat8bppY"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppCb)) { return "WICPixelFormat8bppCb"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppCr)) { return "WICPixelFormat8bppCr"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppCbCr)) { return "WICPixelFormat16bppCbCr"; }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppYQuantizedDctCoefficients))
        {
            return "WICPixelFormat16bppYQuantizedDctCoefficients";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppCbQuantizedDctCoefficients))
        {
            return "WICPixelFormat16bppCbQuantizedDctCoefficients";
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppCrQuantizedDctCoefficients))
        {
            return "WICPixelFormat16bppCrQuantizedDctCoefficients";
        }
        else { return "Unknown WICPixelFormat"; }
    }

    static bool checkImageSignature(const GUID& expectedContainerFormat, std::istream& stream)
    {
        IWICImagingFactory* factory = getFactory();

        if(factory == nullptr) { return false; }

        Microsoft::WRL::ComPtr<IEnumUnknown> decoderEnumerator;
        HRESULT hr = factory->CreateComponentEnumerator(WICDecoder, WICComponentEnumerateDefault, &decoderEnumerator);
        if(FAILED(hr)) return false;

        StdIStream winStream(stream);

        Microsoft::WRL::ComPtr<IUnknown> unknownDecoderInfo;
        ULONG fetchCount;
        hr = decoderEnumerator->Next(1, &unknownDecoderInfo, &fetchCount);

        for(; hr == S_OK && fetchCount == 1; hr = decoderEnumerator->Next(1, &unknownDecoderInfo, &fetchCount))
        {
            Microsoft::WRL::ComPtr<IWICBitmapDecoderInfo> decoderInfo;
            hr = unknownDecoderInfo->QueryInterface(IID_PPV_ARGS(&decoderInfo));
            if(FAILED(hr)) { continue; }

            GUID decoderInfoContainerFormat;
            hr = decoderInfo->GetContainerFormat(&decoderInfoContainerFormat);
            if(FAILED(hr)) { continue; }

            if(!IsEqualGUID(decoderInfoContainerFormat, expectedContainerFormat)) { continue; }

            BOOL matches;
            hr = decoderInfo->MatchesPattern(&winStream, &matches);

            return SUCCEEDED(hr) && matches;
        }

        return false;
    }

    [[nodiscard]] static bool isPaletteFormat(WICPixelFormatGUID pixelFormat)
    {
        return IsEqualGUID(pixelFormat, GUID_WICPixelFormat1bppIndexed) ||
               IsEqualGUID(pixelFormat, GUID_WICPixelFormat2bppIndexed) ||
               IsEqualGUID(pixelFormat, GUID_WICPixelFormat4bppIndexed) ||
               IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppIndexed);
    }

    [[nodiscard]] static bool anyEqualGUID(const GUID& guid1, const GUID& guid2)
    {
        return IsEqualGUID(guid1, guid2);
    }

    template<class... Args>
    [[nodiscard]] static bool anyEqualGUID(const GUID& guid1, const GUID& guid2, const Args&... args)
    {
        return IsEqualGUID(guid1, guid2) || anyEqualGUID(guid1, args...);
    }

    [[nodiscard]] static tl::expected<FormatLayout, TextureImportError>
    wicPixelFormatToLayout(WICPixelFormatGUID pixelFormat, bool paletteHasAlpha, TextureImportOptions options)
    {
        if(IsEqualGUID(pixelFormat, GUID_WICPixelFormatDontCare))
        {
            return tl::make_unexpected(TextureImportError::UnknownFormat);
        }

        if(anyEqualGUID(pixelFormat, GUID_WICPixelFormat1bppIndexed, GUID_WICPixelFormat2bppIndexed,
                        GUID_WICPixelFormat4bppIndexed, GUID_WICPixelFormat8bppIndexed))
        {
            return (paletteHasAlpha || options.padRgbWithAlpha) ? FormatLayout::_8_8_8_8 : FormatLayout::_8_8_8;
        }
        else if(anyEqualGUID(pixelFormat, GUID_WICPixelFormatBlackWhite, GUID_WICPixelFormat2bppGray,
                             GUID_WICPixelFormat4bppGray, GUID_WICPixelFormat8bppGray, GUID_WICPixelFormat24bppBGR,
                             GUID_WICPixelFormat24bppRGB))
        {
            return (options.padRgbWithAlpha) ? FormatLayout::_8_8_8_8 : FormatLayout::_8_8_8;
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat8bppAlpha)) { return FormatLayout::_8; }
        else if(anyEqualGUID(pixelFormat, GUID_WICPixelFormat16bppBGR555, GUID_WICPixelFormat16bppBGRA5551))
        {
            return FormatLayout::_5_5_5_1;
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat16bppBGR565)) { return FormatLayout::_5_6_5; }
        else if(anyEqualGUID(pixelFormat, GUID_WICPixelFormat32bppBGR, GUID_WICPixelFormat32bppRGB,
                             GUID_WICPixelFormat32bppBGRA, GUID_WICPixelFormat32bppPBGRA, GUID_WICPixelFormat32bppRGBA,
                             GUID_WICPixelFormat32bppPRGBA))
        {
            return FormatLayout::_8_8_8_8;
        }
        else if(anyEqualGUID(pixelFormat, GUID_WICPixelFormat32bppBGR101010, GUID_WICPixelFormat32bppRGBA1010102,
                             GUID_WICPixelFormat32bppRGBA1010102XR, GUID_WICPixelFormat32bppR10G10B10A2,
                             GUID_WICPixelFormat32bppR10G10B10A2HDR10))
        {
            return FormatLayout::_10_10_10_2;
        }
        else if(anyEqualGUID(pixelFormat, GUID_WICPixelFormat16bppGray, GUID_WICPixelFormat48bppRGB,
                             GUID_WICPixelFormat48bppBGR, GUID_WICPixelFormat64bppRGB, GUID_WICPixelFormat16bppGrayHalf,
                             GUID_WICPixelFormat64bppRGBHalf))
        {
            return (options.padRgbWithAlpha) ? FormatLayout::_16_16_16_16 : FormatLayout::_16_16_16;
        }
        else if(anyEqualGUID(pixelFormat, GUID_WICPixelFormat64bppRGBAHalf, GUID_WICPixelFormat64bppPRGBAHalf))
        {
            return FormatLayout::_16_16_16_16;
        }
        else if(anyEqualGUID(pixelFormat, GUID_WICPixelFormat32bppGrayFloat, GUID_WICPixelFormat96bppRGBFloat,
                             GUID_WICPixelFormat128bppRGBFloat))
        {
            return (options.padRgbWithAlpha) ? FormatLayout::_32_32_32_32 : FormatLayout::_32_32_32;
        }
        else if(anyEqualGUID(pixelFormat, GUID_WICPixelFormat128bppRGBAFloat, GUID_WICPixelFormat128bppPRGBAFloat))
        {
            return FormatLayout::_32_32_32_32;
        }
        else if(IsEqualGUID(pixelFormat, GUID_WICPixelFormat32bppRGBE)) { return FormatLayout::_9_9_9_5; }

        return tl::make_unexpected(TextureImportError::UnknownFormat);
    }

    [[nodiscard]] static std::span<const FormatLayout> getAllWiderFormatLayouts(FormatLayout formatLayout)
    {
        if(formatLayout == FormatLayout::_5_6_5)
        {
            constexpr std::array layouts = {
                FormatLayout::_8_8_8,       FormatLayout::_8_8_8_8,  FormatLayout::_10_10_10_2, FormatLayout::_16_16_16,
                FormatLayout::_16_16_16_16, FormatLayout::_32_32_32, FormatLayout::_32_32_32_32};
            return layouts;
        }
        else if(formatLayout == FormatLayout::_5_5_5_1)
        {
            constexpr std::array layouts = {FormatLayout::_8_8_8_8, FormatLayout::_10_10_10_2,
                                            FormatLayout::_16_16_16_16, FormatLayout::_32_32_32_32};
            return layouts;
        }
        else if(formatLayout == FormatLayout::_8) // A8
        {
            constexpr std::array layouts = {FormatLayout::_8_8_8_8, FormatLayout::_16_16_16_16,
                                            FormatLayout::_32_32_32_32};
            return layouts;
        }
        else if(formatLayout == FormatLayout::_8_8_8)
        {
            constexpr std::array layouts = {FormatLayout::_8_8_8_8,  FormatLayout::_10_10_10_2,
                                            FormatLayout::_16_16_16, FormatLayout::_16_16_16_16,
                                            FormatLayout::_32_32_32, FormatLayout::_32_32_32_32};
            return layouts;
        }
        else if(formatLayout == FormatLayout::_8_8_8_8)
        {
            constexpr std::array layouts = {FormatLayout::_16_16_16_16, FormatLayout::_32_32_32_32};
            return layouts;
        }
        else if(formatLayout == FormatLayout::_9_9_9_5)
        {
            constexpr std::array layouts = {FormatLayout::_16_16_16, FormatLayout::_16_16_16_16,
                                            FormatLayout::_32_32_32, FormatLayout::_32_32_32_32};
            return layouts;
        }
        else if(formatLayout == FormatLayout::_10_10_10_2)
        {
            constexpr std::array layouts = {FormatLayout::_16_16_16_16, FormatLayout::_32_32_32_32};
            return layouts;
        }
        else if(formatLayout == FormatLayout::_16_16_16)
        {
            constexpr std::array layouts = {FormatLayout::_16_16_16_16, FormatLayout::_32_32_32,
                                            FormatLayout::_32_32_32_32};
            return layouts;
        }
        else if(formatLayout == FormatLayout::_16_16_16_16)
        {
            constexpr std::array layouts = {FormatLayout::_32_32_32_32};
            return layouts;
        }
        else if(formatLayout == FormatLayout::_32_32_32)
        {
            constexpr std::array layouts = {FormatLayout::_32_32_32_32};
            return layouts;
        }

        return {};
    }

    [[nodiscard]] static std::span<const gpufmt::Format> getAvailableFormats(FormatLayout formatLayout,
                                                                             WICPixelFormatGUID imageWicPixelFormat,
                                                                             bool needsAlpha, bool sRgb,
                                                                             std::array<gpufmt::Format, 4>& buffer)
    {
        if(formatLayout == FormatLayout::_5_6_5)
        {
            buffer[0] = gpufmt::Format::R5G6B5_UNORM_PACK16;
            return std::span(buffer.cbegin(), 1);
        }
        else if(formatLayout == FormatLayout::_5_5_5_1)
        {
            buffer[0] = gpufmt::Format::R5G5B5A1_UNORM_PACK16;
            return std::span(buffer.cbegin(), 1);
        }
        else if(formatLayout == FormatLayout::_8)
        {
            buffer[0] = gpufmt::Format::A8_UNORM;
            return std::span(buffer.cbegin(), 1);
        }
        else if(formatLayout == FormatLayout::_8_8_8)
        {
            buffer[0] = (sRgb) ? gpufmt::Format::R8G8B8_SRGB : gpufmt::Format::R8G8B8_UNORM;
            buffer[1] = (sRgb) ? gpufmt::Format::B8G8R8_SRGB : gpufmt::Format::B8G8R8_UNORM;

            if(IsEqualGUID(imageWicPixelFormat, GUID_WICPixelFormat24bppBGR)) { std::swap(buffer[0], buffer[1]); }

            return std::span(buffer.cbegin(), 2);
        }
        else if(formatLayout == FormatLayout::_8_8_8_8)
        {
            if((IsEqualGUID(imageWicPixelFormat, GUID_WICPixelFormat32bppBGR) && needsAlpha) ||
               IsEqualGUID(imageWicPixelFormat, GUID_WICPixelFormat32bppBGRA))
            {
                buffer[0] = (sRgb) ? gpufmt::Format::B8G8R8A8_SRGB : gpufmt::Format::B8G8R8A8_UNORM;
                buffer[1] = (sRgb) ? gpufmt::Format::R8G8B8A8_SRGB : gpufmt::Format::R8G8B8A8_UNORM;
                buffer[2] = (sRgb) ? gpufmt::Format::A8B8G8R8_SRGB_PACK32 : gpufmt::Format::A8B8G8R8_UNORM_PACK32;
                return std::span(buffer.cbegin(), 3);
            }
            else if(IsEqualGUID(imageWicPixelFormat, GUID_WICPixelFormat32bppBGR) && !needsAlpha)
            {
                buffer[0] = (sRgb) ? gpufmt::Format::B8G8R8X8_SRGB : gpufmt::Format::B8G8R8X8_UNORM;
                buffer[1] = (sRgb) ? gpufmt::Format::B8G8R8A8_SRGB : gpufmt::Format::B8G8R8A8_UNORM;
                buffer[2] = (sRgb) ? gpufmt::Format::R8G8B8A8_SRGB : gpufmt::Format::R8G8B8A8_UNORM;
                buffer[3] = (sRgb) ? gpufmt::Format::A8B8G8R8_SRGB_PACK32 : gpufmt::Format::A8B8G8R8_UNORM_PACK32;
                return std::span(buffer.cbegin(), 4);
            }
            else if(IsEqualGUID(imageWicPixelFormat, GUID_WICPixelFormat32bppRGBA))
            {
                buffer[0] = (sRgb) ? gpufmt::Format::R8G8B8A8_SRGB : gpufmt::Format::R8G8B8A8_UNORM;
                buffer[1] = (sRgb) ? gpufmt::Format::B8G8R8A8_SRGB : gpufmt::Format::B8G8R8A8_UNORM;
                buffer[2] = (sRgb) ? gpufmt::Format::A8B8G8R8_SRGB_PACK32 : gpufmt::Format::A8B8G8R8_UNORM_PACK32;
                return std::span(buffer.cbegin(), 3);
            }
            else if(IsEqualGUID(imageWicPixelFormat, GUID_WICPixelFormat32bppPRGBA))
            {
                buffer[0] = (sRgb) ? gpufmt::Format::A8B8G8R8_SRGB_PACK32 : gpufmt::Format::A8B8G8R8_UNORM_PACK32;
                buffer[1] = (sRgb) ? gpufmt::Format::R8G8B8A8_SRGB : gpufmt::Format::R8G8B8A8_UNORM;
                buffer[2] = (sRgb) ? gpufmt::Format::B8G8R8A8_SRGB : gpufmt::Format::B8G8R8A8_UNORM;
                return std::span(buffer.cbegin(), 3);
            }
            else
            {
                buffer[0] = (sRgb) ? gpufmt::Format::R8G8B8A8_SRGB : gpufmt::Format::R8G8B8A8_UNORM;
                buffer[1] = (sRgb) ? gpufmt::Format::B8G8R8A8_SRGB : gpufmt::Format::B8G8R8A8_UNORM;
                buffer[2] = (sRgb) ? gpufmt::Format::A8B8G8R8_SRGB_PACK32 : gpufmt::Format::A8B8G8R8_UNORM_PACK32;
                return std::span(buffer.cbegin(), 3);
            }
        }
        else if(formatLayout == FormatLayout::_9_9_9_5)
        {
            buffer[0] = gpufmt::Format::E5B9G9R9_UFLOAT_PACK32;
            return std::span(buffer.cbegin(), 1);
        }
        else if(formatLayout == FormatLayout::_10_10_10_2)
        {
            buffer[0] = gpufmt::Format::A2B10G10R10_UNORM_PACK32;
            return std::span(buffer.cbegin(), 1);
        }
        else if(formatLayout == FormatLayout::_16_16_16)
        {
            buffer[0] = gpufmt::Format::R16G16B16_UNORM;
            buffer[1] = gpufmt::Format::R16G16B16_SFLOAT;

            if(anyEqualGUID(imageWicPixelFormat, GUID_WICPixelFormat64bppRGBHalf, GUID_WICPixelFormat48bppRGBHalf,
                            GUID_WICPixelFormat16bppGrayHalf))
            {
                std::swap(buffer[0], buffer[1]);
            }

            return std::span(buffer.cbegin(), 2);
        }
        else if(formatLayout == FormatLayout::_16_16_16_16)
        {
            buffer[0] = gpufmt::Format::R16G16B16A16_UNORM;
            buffer[1] = gpufmt::Format::R16G16B16A16_SFLOAT;

            if(anyEqualGUID(imageWicPixelFormat, GUID_WICPixelFormat64bppRGBHalf, GUID_WICPixelFormat48bppRGBHalf,
                            GUID_WICPixelFormat16bppGrayHalf, GUID_WICPixelFormat64bppRGBAHalf,
                            GUID_WICPixelFormat64bppPRGBAHalf))
            {
                std::swap(buffer[0], buffer[1]);
            }

            return std::span(buffer.cbegin(), 2);
        }
        else if(formatLayout == FormatLayout::_32_32_32)
        {
            buffer[0] = gpufmt::Format::R32G32B32_SFLOAT;
            return std::span(buffer.cbegin(), 1);
        }
        else if(formatLayout == FormatLayout::_32_32_32_32)
        {
            buffer[0] = gpufmt::Format::R32G32B32A32_SFLOAT;
            return std::span(buffer.cbegin(), 1);
        }
        else { return {}; }
    }

    [[nodiscard]] static WICPixelFormatGUID gpuFormatToWICPixelFormat(gpufmt::Format format)
    {
        switch(format)
        {
        case gpufmt::Format::R5G6B5_UNORM_PACK16: return GUID_WICPixelFormat16bppBGR565;
        case gpufmt::Format::R5G5B5A1_UNORM_PACK16: return GUID_WICPixelFormat16bppBGRA5551;
        case gpufmt::Format::A8_UNORM: return GUID_WICPixelFormat8bppAlpha;
        case gpufmt::Format::R8G8B8_UNORM: return GUID_WICPixelFormat24bppBGR;
        case gpufmt::Format::R8G8B8_SRGB: return GUID_WICPixelFormat24bppBGR;
        case gpufmt::Format::B8G8R8_UNORM: return GUID_WICPixelFormat24bppRGB;
        case gpufmt::Format::B8G8R8_SRGB: return GUID_WICPixelFormat24bppRGB;
        case gpufmt::Format::R8G8B8A8_UNORM: return GUID_WICPixelFormat32bppRGBA;
        case gpufmt::Format::R8G8B8A8_SRGB: return GUID_WICPixelFormat32bppRGBA;
        case gpufmt::Format::B8G8R8A8_UNORM: return GUID_WICPixelFormat32bppBGRA;
        case gpufmt::Format::B8G8R8A8_SRGB: return GUID_WICPixelFormat32bppBGRA;
        case gpufmt::Format::B8G8R8X8_UNORM: return GUID_WICPixelFormat32bppBGR;
        case gpufmt::Format::B8G8R8X8_SRGB: return GUID_WICPixelFormat32bppBGR;
        case gpufmt::Format::A8B8G8R8_UNORM_PACK32: return GUID_WICPixelFormat32bppPRGBA;
        case gpufmt::Format::A8B8G8R8_SRGB_PACK32: return GUID_WICPixelFormat32bppPRGBA;
        case gpufmt::Format::A2B10G10R10_UNORM_PACK32: return GUID_WICPixelFormat32bppRGBA1010102;
        case gpufmt::Format::R16G16B16_UNORM: return GUID_WICPixelFormat48bppRGB;
        case gpufmt::Format::R16G16B16_SFLOAT: return GUID_WICPixelFormat48bppRGBHalf;
        case gpufmt::Format::R16G16B16A16_UNORM: return GUID_WICPixelFormat64bppRGBA;
        case gpufmt::Format::R16G16B16A16_SFLOAT: return GUID_WICPixelFormat64bppRGBAHalf;
        case gpufmt::Format::R32G32B32_SFLOAT: return GUID_WICPixelFormat96bppRGBFloat;
        case gpufmt::Format::R32G32B32A32_SFLOAT: return GUID_WICPixelFormat96bppRGBFloat;
        case gpufmt::Format::E5B9G9R9_UFLOAT_PACK32: return GUID_WICPixelFormat32bppRGBE;
        default: return GUID_WICPixelFormatUndefined;
        }
    }

private:
    static Microsoft::WRL::ComPtr<IWICImagingFactory> mFactory;
};
} // namespace teximp

#endif // TEXIMP_PLATFORM_WINDOWS