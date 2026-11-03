
#include <atlbase.h>
#include <wincodec.h>

#include "d3d11_spine_texture_loader.h"

#include <spine/Atlas.h> /* spine::AtlasPage */

#if defined(_MSVC_LANG)
	#pragma comment (lib, "Windowscodecs.lib")
#endif

/* Internal static variables and functions; no namespaces */

static bool g_toLoadTextureAsBgra32Format = false;
static bool g_toLoadTextureAsPma = false;

static const WICPixelFormatGUID GetGuidFormatToLoad()
{
	return g_toLoadTextureAsBgra32Format
		? g_toLoadTextureAsPma
		? GUID_WICPixelFormat32bppPBGRA : GUID_WICPixelFormat32bppBGRA
		: g_toLoadTextureAsPma
		? GUID_WICPixelFormat32bppPRGBA : GUID_WICPixelFormat32bppRGBA;
}

static bool LoadImageAsWicBitmapFromFile(const wchar_t* filePath, IWICBitmap** ppWicBitmap)
{
	if (ppWicBitmap == nullptr)return false;

	CComPtr<IWICImagingFactory> pWicImageFactory;
	HRESULT hr = ::CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWicImageFactory));
	if (FAILED(hr)) return false;

	CComPtr<IWICBitmapDecoder> pWicBitmapDecoder;
	hr = pWicImageFactory->CreateDecoderFromFilename(filePath, nullptr, GENERIC_READ, WICDecodeMetadataCacheOnDemand, &pWicBitmapDecoder);
	if (FAILED(hr)) return false;

	CComPtr<IWICBitmapFrameDecode> pWicFrameDecode;
	hr = pWicBitmapDecoder->GetFrame(0, &pWicFrameDecode);
	if (FAILED(hr))return false;

	CComPtr<IWICFormatConverter> pWicFormatConverter;
	hr = pWicImageFactory->CreateFormatConverter(&pWicFormatConverter);
	if (FAILED(hr))return false;

	hr = pWicFormatConverter->Initialize(pWicFrameDecode,
		GetGuidFormatToLoad(),
		WICBitmapDitherTypeNone, nullptr, 0.f, WICBitmapPaletteTypeCustom);
	if (FAILED(hr))return false;

	hr = pWicImageFactory->CreateBitmapFromSource(pWicFormatConverter, WICBitmapCacheOnDemand, ppWicBitmap);

	return SUCCEEDED(hr);
}

/// @brief WIC画素配列転送メモリ管理
class CWicBitmapPixels
{
public:
	CWicBitmapPixels() noexcept = default;
	explicit CWicBitmapPixels(IWICBitmap* pWixBitmap) noexcept
	{
		readPixels(pWixBitmap);
	}
	~CWicBitmapPixels() noexcept
	{
		release();
	}

	bool readPixels(IWICBitmap* pWicBitmap) noexcept
	{
		HRESULT hr = pWicBitmap->GetSize(&m_width, &m_height);
		if (FAILED(hr))return false;

		CComPtr<IWICBitmapLock> pWicBitmapLock;
		const WICRect wicRect{ 0, 0, static_cast<INT>(m_width), static_cast<INT>(m_height) };
		hr = pWicBitmap->Lock(&wicRect, WICBitmapLockRead, &pWicBitmapLock);
		if (FAILED(hr))return false;

		hr = pWicBitmapLock->GetStride(&m_stride);
		if (FAILED(hr))return false;

		const size_t nSize = static_cast<size_t>(m_stride * m_height);
		m_pPixels = static_cast<BYTE*>(::malloc(nSize));
		if (m_pPixels == nullptr)return false;

		hr = pWicBitmap->CopyPixels(nullptr, m_stride, static_cast<UINT>(nSize), m_pPixels);
		if (FAILED(hr))return false;

		return true;
	}

	bool isValid() const noexcept { return m_pPixels != nullptr; }

	const BYTE* pixels() const noexcept { return m_pPixels; }
	UINT width() const noexcept { return m_width; }
	UINT height() const noexcept { return m_height; }
	UINT stride() const noexcept { return m_stride; }
private:
	BYTE* m_pPixels = nullptr;
	UINT m_width = 0;
	UINT m_height = 0;
	UINT m_stride = 0;

	void release() noexcept
	{
		if (m_pPixels != nullptr)
		{
			::free(m_pPixels);
			m_pPixels = nullptr;
		}
	}
};

static bool CreateD3D11TextureFromWicBitmap(ID3D11Device* pD3D11Device, IWICBitmap* pWicBitmap, ID3D11Texture2D** ppD3D11Texture2D)
{
	if (ppD3D11Texture2D == nullptr)return false;

	CWicBitmapPixels wicBitmapPixels(pWicBitmap);
	if (!wicBitmapPixels.isValid())return false;

	const CD3D11_TEXTURE2D_DESC desc(
		g_toLoadTextureAsBgra32Format ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM,
		wicBitmapPixels.width(),
		wicBitmapPixels.height(),
		1,
		1);

	const D3D11_SUBRESOURCE_DATA subResource(
		wicBitmapPixels.pixels(),
		wicBitmapPixels.stride(),
		0);

	HRESULT hr = pD3D11Device->CreateTexture2D(&desc, &subResource, ppD3D11Texture2D);

	return SUCCEEDED(hr);
}

static bool CreateShaderResourceViewFromTexture2D(ID3D11Device* pD3D11Device, ID3D11Texture2D* pD3D11Texture2D, ID3D11ShaderResourceView** ppD3D11ShaderResourceView)
{
	if (ppD3D11ShaderResourceView == nullptr)return false;

	D3D11_TEXTURE2D_DESC d3d11Texture2dDesc;
	pD3D11Texture2D->GetDesc(&d3d11Texture2dDesc);

	const D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceVireDesc
	{
		d3d11Texture2dDesc.Format,
		D3D11_SRV_DIMENSION_TEXTURE2D,
		{0, 1} /* D3D11_TEX2D_SRV */
	};
	HRESULT hr = pD3D11Device->CreateShaderResourceView(pD3D11Texture2D, &shaderResourceVireDesc, ppD3D11ShaderResourceView);

	return SUCCEEDED(hr);
}

/* End of internal static variables and functions */

void CD3D11SpineTextureLoader::SetFormatAsBgraOnLoading(bool toLoadAsBgra32) noexcept
{
	g_toLoadTextureAsBgra32Format = toLoadAsBgra32;
}

bool CD3D11SpineTextureLoader::IsTextureToBeLoadedAsBgra32() noexcept
{
	return g_toLoadTextureAsBgra32Format;
}

void CD3D11SpineTextureLoader::EnableConversionToPma(bool toLoadAsPma) noexcept
{
	g_toLoadTextureAsPma = toLoadAsPma;
}

bool CD3D11SpineTextureLoader::IsConversionToPmaEnabled() noexcept
{
	return g_toLoadTextureAsPma;
}

void CD3D11SpineTextureLoader::load(spine::AtlasPage& atlasPage, const spine::String& textureFilePath)
{
	if (m_pStotedD3D11Device == nullptr)return;

#if defined(SPINE_40) || defined(SPINE_41) || defined (SPINE_42)
	/* Load as PBGRA only when spine::AtlasPage::pma is false. */
	if (!atlasPage.pma)
	{
		if (g_toLoadTextureAsPma)
		{
			atlasPage.pma = true;
		}
	}
	else
	{
		g_toLoadTextureAsPma = false;
	}
#endif

	CComPtr<IWICBitmap> pWicBitmap;
	if (m_pTextureLoadCallback != nullptr)
	{
		m_pTextureLoadCallback(m_pCallbackUserDatum, textureFilePath.buffer(), textureFilePath.length(), &pWicBitmap);
	}

	if (pWicBitmap == nullptr)
	{
		static constexpr size_t kMaxFilePathLength = 1024;
		wchar_t filePathBuffer[kMaxFilePathLength]{};
		static constexpr size_t kFilePathBufferSize = sizeof(filePathBuffer) / sizeof(filePathBuffer[0]);

		int nWritten = ::MultiByteToWideChar(CP_UTF8, 0, textureFilePath.buffer(), textureFilePath.length(), filePathBuffer, kFilePathBufferSize);
		if (nWritten == 0)return;

		LoadImageAsWicBitmapFromFile(filePathBuffer, &pWicBitmap);
		if (pWicBitmap == nullptr)return;
	}

	CComPtr<ID3D11Texture2D> pD3D11Texture2D;
	CreateD3D11TextureFromWicBitmap(m_pStotedD3D11Device, pWicBitmap, &pD3D11Texture2D);
	if (pD3D11Texture2D == nullptr)return;

	ID3D11ShaderResourceView* pD3D11ShaderResourceView = nullptr;
	CreateShaderResourceViewFromTexture2D(m_pStotedD3D11Device, pD3D11Texture2D, &pD3D11ShaderResourceView);

#if defined (SPINE_41) || defined (SPINE_42)
	atlasPage.texture = pD3D11ShaderResourceView;
#else
	atlasPage.setRendererObject(pD3D11ShaderResourceView);
#endif
}

void CD3D11SpineTextureLoader::unload(void* texture)
{
	ID3D11ShaderResourceView* pD3D11ShaderResourceView = static_cast<ID3D11ShaderResourceView*>(texture);
	if (pD3D11ShaderResourceView != nullptr)
	{
#if defined(_DEBUG)
		CComPtr<ID3D11Resource> pD3D11Resource;
		pD3D11ShaderResourceView->GetResource(&pD3D11Resource);
		if (pD3D11Resource != nullptr)
		{
			CComPtr<ID3D11Texture2D> pD3D11Texture2D;
			pD3D11Resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pD3D11Texture2D);
			if (pD3D11Texture2D != nullptr)
			{
				D3D11_TEXTURE2D_DESC d3d11Texture2dDesc;
				pD3D11Texture2D->GetDesc(&d3d11Texture2dDesc);
				wchar_t buffer[1024]{};
				::swprintf_s(buffer, L"Release texture; width: %u, height: %u, DXGI format: %u\n", d3d11Texture2dDesc.Width, d3d11Texture2dDesc.Height, d3d11Texture2dDesc.Format);
				::OutputDebugStringW(buffer);
			}
		}
#endif
		pD3D11ShaderResourceView->Release();
	}
}

void CD3D11SpineTextureLoader::setD3D11Device(ID3D11Device* pD3D11Device) noexcept
{
	m_pStotedD3D11Device = pD3D11Device;
}

void CD3D11SpineTextureLoader::setTextureLoadCallback(LoadCallback pFunc, void* pUserDatum) noexcept
{
	m_pTextureLoadCallback = pFunc;
	m_pCallbackUserDatum = pUserDatum;
}
