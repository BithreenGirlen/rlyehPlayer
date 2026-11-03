

#include "d3d11_core.h"

#if defined(_MSVC_LANG)
	#pragma comment (lib, "d3d11.lib")
	#pragma comment (lib, "dxguid.lib")
	#pragma comment (lib, "D2d1.lib")
#endif

bool CD3D11Core::initialise(HWND hRenderWindow, bool toBeUsedWithDirect2D) noexcept
{
	releaseAll();
	if (hRenderWindow == nullptr)return false;

	D3D_FEATURE_LEVEL featureLevel{};
	HRESULT hr = ::D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
		(toBeUsedWithDirect2D ? D3D11_CREATE_DEVICE_BGRA_SUPPORT : 0) | D3D11_CREATE_DEVICE_SINGLETHREADED,
		nullptr, 0, D3D11_SDK_VERSION,
		&m_pD3d11Device, &featureLevel, &m_pD3d11DeviceContext);
	if (FAILED(hr))return false;

	CComPtr<IDXGIDevice1> pDxgiDevice1;
	hr = m_pD3d11Device->QueryInterface(__uuidof(IDXGIDevice1), (void**)&pDxgiDevice1);
	if (FAILED(hr))return false;

	hr = pDxgiDevice1->SetMaximumFrameLatency(1);
	if (FAILED(hr))return false;

	CComPtr<IDXGIAdapter> pDxgiAdapter;
	hr = pDxgiDevice1->GetAdapter(&pDxgiAdapter);
	if (FAILED(hr))return false;

	CComPtr<IDXGIFactory2> pDxgiFactory2;
	hr = pDxgiAdapter->GetParent(IID_PPV_ARGS(&pDxgiFactory2));
	if (FAILED(hr))return false;

	RECT rc{};
	::GetClientRect(hRenderWindow, &rc);

	const DXGI_SWAP_CHAIN_DESC1 swapChainDesc
	{
		static_cast<UINT>(rc.right - rc.left),
		static_cast<UINT>(rc.bottom - rc.top),
		toBeUsedWithDirect2D ? DXGI_FORMAT_B8G8R8A8_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM,
		FALSE,
		{1, 0},
		DXGI_USAGE_RENDER_TARGET_OUTPUT,
		2,
		DXGI_SCALING::DXGI_SCALING_NONE,
		DXGI_SWAP_EFFECT::DXGI_SWAP_EFFECT_FLIP_DISCARD,
		DXGI_ALPHA_MODE::DXGI_ALPHA_MODE_IGNORE,
		0
	};

	hr = pDxgiFactory2->CreateSwapChainForHwnd(pDxgiDevice1, hRenderWindow, &swapChainDesc, nullptr, nullptr, &m_pDxgiSwapChain1);
	if (FAILED(hr))return false;

	updateRefreshRate();

	return toBeUsedWithDirect2D ? initialiseDirect2D(pDxgiDevice1) : true;
}

bool CD3D11Core::hasDirect3DBeenInitialised() const noexcept
{
	return m_pD3d11Device != nullptr && m_pD3d11DeviceContext != nullptr && m_pDxgiSwapChain1 != nullptr;
}

bool CD3D11Core::hasDirect2DBeenInitialised() const noexcept
{
	return m_pD2d1Factory1 != nullptr && m_pD2d1DeviceContext != nullptr;
}

void CD3D11Core::bindRenderTarget(ID3D11RenderTargetView* pRenderTargetView) noexcept
{
	if (m_pD3d11DeviceContext == nullptr)return;

	m_pD3d11DeviceContext->OMSetRenderTargets(1, pRenderTargetView == nullptr ? &m_pBackBufferRenderTargetView.p : &pRenderTargetView, nullptr);
}

void CD3D11Core::clear(const D2D_VECTOR_4F& colour) noexcept
{
	if (m_pD3d11DeviceContext == nullptr)return;

	CComPtr<ID3D11RenderTargetView> pRendertargetView;
	m_pD3d11DeviceContext->OMGetRenderTargets(1, &pRendertargetView, nullptr);
	if (pRendertargetView == nullptr)return;

	const FLOAT clearColour[4] = { colour.x, colour.y, colour.z, colour.w };
	m_pD3d11DeviceContext->ClearRenderTargetView(pRendertargetView, clearColour);
}

void CD3D11Core::display() noexcept
{
	if (m_pDxgiSwapChain1 != nullptr)
	{
		HRESULT hr = m_pDxgiSwapChain1->Present(1, 0);
		if (hr == DXGI_STATUS_OCCLUDED)
		{
			::Sleep(static_cast<DWORD>(1000.f / m_displayRefreshRate));
		}
	}
}

bool CD3D11Core::createD3CompatibleD2Bitmap(const D2D1_SIZE_U& size, ID2D1Bitmap1** pD2RenderTarget, ID3D11ShaderResourceView** pD3TargetResourceView) noexcept
{
	if (m_pD3d11Device == nullptr || m_pD2d1DeviceContext == nullptr)return false;

	const CD3D11_TEXTURE2D_DESC d3Texture2dDesc(
		DXGI_FORMAT_B8G8R8A8_UNORM,
		size.width,
		size.height,
		1,
		1,
		D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE);

	CComPtr<ID3D11Texture2D> pD3D11Texture2D;
	HRESULT hr = m_pD3d11Device->CreateTexture2D(&d3Texture2dDesc, nullptr, &pD3D11Texture2D);
	if (FAILED(hr))return false;

	CComPtr<IDXGISurface> pTexture2dSurface;
	hr = pD3D11Texture2D->QueryInterface(IID_PPV_ARGS(&pTexture2dSurface));
	if (FAILED(hr))return false;

	const D2D1_BITMAP_PROPERTIES1 bitmapProperties1 = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS::D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE));

	hr = m_pD2d1DeviceContext->CreateBitmapFromDxgiSurface(pTexture2dSurface, &bitmapProperties1, pD2RenderTarget);
	if (FAILED(hr))return false;

	const D3D11_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc
	{
		DXGI_FORMAT_B8G8R8A8_UNORM,
		D3D11_SRV_DIMENSION_TEXTURE2D,
		{0, 1} /* D3D11_TEX2D_SRV */
	};
	hr = m_pD3d11Device->CreateShaderResourceView(pD3D11Texture2D, &shaderResourceViewDesc, pD3TargetResourceView);
	if (FAILED(hr))return false;

	return true;
}

ID3D11Device* const CD3D11Core::getD3d11Device() const noexcept
{
	return m_pD3d11Device;
};

ID3D11DeviceContext* const CD3D11Core::getD3D11DeviceContext() const noexcept
{
	return m_pD3d11DeviceContext;
};

IDXGISwapChain1* const CD3D11Core::getSwapChain() const noexcept
{
	return m_pDxgiSwapChain1;
}

ID2D1Factory1* const CD3D11Core::getD2Factory() const noexcept
{
	return m_pD2d1Factory1;
}

ID2D1DeviceContext* const CD3D11Core::getD2DeviceContext() const noexcept
{
	return m_pD2d1DeviceContext;
}

void CD3D11Core::onResize() noexcept
{
	checkBackBufferSize();
}

void CD3D11Core::releaseAll() noexcept
{
	m_pD3d11Device.Release();
	m_pD3d11DeviceContext.Release();
	m_pDxgiSwapChain1.Release();

	m_pBackBufferRenderTargetView.Release();
	m_backBufferWidth = 0;
	m_backBufferHeight = 0;

	m_pD2d1Factory1.Release();
	m_pD2d1DeviceContext.Release();
}

void CD3D11Core::updateRefreshRate() noexcept
{
	if (m_pDxgiSwapChain1 == nullptr)return;

	HWND hWnd = nullptr;
	HRESULT hr = m_pDxgiSwapChain1->GetHwnd(&hWnd);
	if (FAILED(hr))return;

	MONITORINFOEX  monitorInfoEx{ sizeof(MONITORINFOEX) };
	BOOL iRet = 0;
	HMONITOR hMonitor = ::MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor != nullptr)
	{
		iRet = ::GetMonitorInfoW(hMonitor, &monitorInfoEx);
	}

	const bool hasObtainedDeviceName = (hMonitor != nullptr) && (iRet != 0);
	DEVMODE devMode{};
	iRet = ::EnumDisplaySettingsW(hasObtainedDeviceName ? nullptr : monitorInfoEx.szDevice, ENUM_CURRENT_SETTINGS, &devMode);
	if (iRet != 0)
	{
		m_displayRefreshRate = devMode.dmDisplayFrequency;
	}
}

bool CD3D11Core::initialiseDirect2D(IDXGIDevice1* pDxGiDevice1) noexcept
{
	HRESULT hr = ::D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pD2d1Factory1);
	if (FAILED(hr))return false;

	CComPtr<ID2D1Device> pD2d1Device;
	hr = m_pD2d1Factory1->CreateDevice(pDxGiDevice1, &pD2d1Device);
	if (FAILED(hr))return false;

	hr = pD2d1Device->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pD2d1DeviceContext);
	if (FAILED(hr))return false;

	m_pD2d1DeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
	m_pD2d1DeviceContext->SetAntialiasMode(D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
	m_pD2d1DeviceContext->SetUnitMode(D2D1_UNIT_MODE_DIPS);
	m_pD2d1DeviceContext->SetPrimitiveBlend(D2D1_PRIMITIVE_BLEND_SOURCE_OVER);

	D2D1_RENDERING_CONTROLS renderingControls{};
	m_pD2d1DeviceContext->GetRenderingControls(&renderingControls);

	renderingControls.bufferPrecision = D2D1_BUFFER_PRECISION_8BPC_UNORM_SRGB;
	m_pD2d1DeviceContext->SetRenderingControls(renderingControls);

	return true;
}

bool CD3D11Core::checkBackBufferSize() noexcept
{
	if (m_pDxgiSwapChain1 == nullptr)return false;

	HWND hWnd = nullptr;
	HRESULT hr = m_pDxgiSwapChain1->GetHwnd(&hWnd);
	if (FAILED(hr))return false;

	RECT rc;
	::GetClientRect(hWnd, &rc);

	const UINT clientWidth = rc.right - rc.left;
	const UINT clientHeight = rc.bottom - rc.top;

	if (m_backBufferWidth != clientWidth || m_backBufferHeight != clientHeight)
	{
		return recreateBackBuffer(clientWidth, clientHeight);
	}

	return true;
}

bool CD3D11Core::recreateBackBuffer(UINT width, UINT height) noexcept
{
	if (m_pD3d11Device == nullptr || m_pD3d11DeviceContext == nullptr)return false;

	unbindAllRenderTargets();
	m_pBackBufferRenderTargetView.Release();

	/* Direct2D may be used, so retrieve the format. */
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc1;
	HRESULT hr = m_pDxgiSwapChain1->GetDesc1(&swapChainDesc1);
	hr = m_pDxgiSwapChain1->ResizeBuffers(0, width, height, swapChainDesc1.Format, 0);
	if (FAILED(hr))return false;

	m_backBufferWidth = width;
	m_backBufferHeight = height;

	CComPtr<ID3D11Texture2D> pDirect3dBackBuffer;
	hr = m_pDxgiSwapChain1->GetBuffer(0, IID_PPV_ARGS(&pDirect3dBackBuffer));
	if (FAILED(hr))return false;

	hr = m_pD3d11Device->CreateRenderTargetView(pDirect3dBackBuffer, nullptr, &m_pBackBufferRenderTargetView);
	if (FAILED(hr))return false;

#if 0 /* All the drawing by Direct2D should be done on ID2D1Bitmap queried from ID3D11Texture2D. */
	if (m_pD2d1DeviceContext != nullptr)
	{
		CComPtr<IDXGISurface> pDirect2dBackBuffer;
		hr = m_pDxgiSwapChain1->GetBuffer(0, IID_PPV_ARGS(&pDirect2dBackBuffer));
		if (FAILED(hr))return false;

		const D2D1_BITMAP_PROPERTIES1 d2d1BitmapProperty
		{
			{swapChainDesc1.Format, D2D1_ALPHA_MODE_IGNORE},
			96.f, 96.f, 
			D2D1_BITMAP_OPTIONS::D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS::D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
			nullptr
		};
		CComPtr<ID2D1Bitmap1> pD2d1Bitmap1;
		hr = m_pD2d1DeviceContext->CreateBitmapFromDxgiSurface(pDirect2dBackBuffer, &d2d1BitmapProperty, &pD2d1Bitmap1);
		if (FAILED(hr))return false;

		m_pD2d1DeviceContext->SetTarget(pD2d1Bitmap1);
	}
#endif

	bindRenderTarget(m_pBackBufferRenderTargetView.p);

	return true;
}

void CD3D11Core::unbindAllRenderTargets() noexcept
{
	ID3D11RenderTargetView* pNullRenderTaregts[] = { nullptr };
	static constexpr UINT kNullRenderTargetsSize = sizeof(pNullRenderTaregts) / sizeof(pNullRenderTaregts[0]);
	m_pD3d11DeviceContext->OMSetRenderTargets(kNullRenderTargetsSize, pNullRenderTaregts, nullptr);

	if (m_pD2d1DeviceContext != nullptr)
	{
		m_pD2d1DeviceContext->SetTarget(nullptr);
	}
}
