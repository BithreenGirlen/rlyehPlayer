#ifndef D3D11_CORE_H_
#define D3D11_CORE_H_

#include <Windows.h>
#include <dxgi1_2.h>
#include <d3d11.h>
#include <d2d1_1.h>
#include <atlbase.h>

/// @brief Manages Direct3D11 and Direct2D/DirectWrite compatibility
class CD3D11Core
{
public:
	CD3D11Core() noexcept = default;
	~CD3D11Core() noexcept = default;

	/// @brief Pass nullptr as hWnd to release all the owned data
	bool initialise(HWND hRenderWindow, bool toBeUsedWithDirect2D) noexcept;
	bool hasDirect3DBeenInitialised() const noexcept;
	bool hasDirect2DBeenInitialised() const noexcept;

	/// @brief Pass nullptr to set back-buffer as render-target.
	void bindRenderTarget(ID3D11RenderTargetView* pRenderTargetView) noexcept;

	void clear(const D2D_VECTOR_4F& colour = D2D_VECTOR_4F(0.f, 0.f, 0.f, 1.f)) noexcept;
	void display() noexcept;

	bool createD3CompatibleD2Bitmap(const D2D1_SIZE_U& size, ID2D1Bitmap1** pD2RenderTarget, ID3D11ShaderResourceView** pD3TargetResourceView) noexcept;

	ID3D11Device* const getD3d11Device() const noexcept;
	ID3D11DeviceContext* const getD3D11DeviceContext() const noexcept;
	IDXGISwapChain1* const getSwapChain() const noexcept;

	ID2D1Factory1* const getD2Factory()const noexcept;
	ID2D1DeviceContext* const getD2DeviceContext()const noexcept;

	void onResize() noexcept;
private:
	CComPtr<ID3D11Device> m_pD3d11Device;
	CComPtr<ID3D11DeviceContext> m_pD3d11DeviceContext;
	CComPtr<IDXGISwapChain1> m_pDxgiSwapChain1;

	CComPtr<ID3D11RenderTargetView> m_pBackBufferRenderTargetView;

	UINT m_backBufferWidth = 0;
	UINT m_backBufferHeight = 0;

	CComPtr<ID2D1Factory1> m_pD2d1Factory1;
	CComPtr<ID2D1DeviceContext> m_pD2d1DeviceContext;

	DWORD m_displayRefreshRate = 60;

	void releaseAll() noexcept;

	void updateRefreshRate() noexcept;
	bool initialiseDirect2D(IDXGIDevice1* pDxGiDevice1) noexcept;

	bool checkBackBufferSize() noexcept;
	bool recreateBackBuffer(UINT width, UINT height) noexcept;
	void unbindAllRenderTargets() noexcept;
};

#endif // !D3D11_CORE_H_
