

#include <algorithm>

#include "main_window.h"

#include "win_dialogue.h"
#include "win_filesystem.h"
#include "win_text.h"
#include "native-ui/window_menu.h"
#include "d3d11_spine/spine_file_verifier.h"
#include "rlyeh.h"


bool CMainWindow::create(HINSTANCE hInstance, const wchar_t* windowName, HICON hIcon)
{
	WNDCLASSEXW wcex{};

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = ::GetSysColorBrush(COLOR_BTNFACE);
	wcex.lpszClassName = m_className;
	if (hIcon != nullptr)
	{
		wcex.hIcon = hIcon;
		wcex.hIconSm = hIcon;
	}

	if (::RegisterClassExW(&wcex))
	{
		m_hInstance = hInstance;

		m_hWnd = ::CreateWindowW(m_className, windowName == nullptr ? m_defaultWindowName : windowName, WS_OVERLAPPEDWINDOW & ~WS_MINIMIZEBOX & ~WS_MAXIMIZEBOX & ~WS_THICKFRAME,
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, nullptr, nullptr, hInstance, this);
		if (m_hWnd != nullptr)
		{
			return true;
		}
		else
		{
			showErrorMessageBox(L"CreateWindowW failed; code: %ul", ::GetLastError());
		}
	}
	else
	{
		showErrorMessageBox(L"RegisterClassExW failed; code: %ul", ::GetLastError());
	}

	return false;
}

int CMainWindow::messageLoop()
{
	MSG msg{};
	for (; msg.message != WM_QUIT;)
	{
		/* Reduce burden on CPU when nothing has to be drawn. */
		BOOL iRet = m_pD3d11SpinePlayer != nullptr && m_pD3d11SpinePlayer->hasSpineBeenLoaded() ?
			::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE) :
			::GetMessageW(&msg, nullptr, 0, 0);
		if (iRet)
		{
			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		}

		tick();
	}

	return static_cast<int>(msg.wParam);
}
/* C CALLBACK */
LRESULT CMainWindow::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	CMainWindow* pThis = nullptr;
	if (uMsg == WM_NCCREATE)
	{
		LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
		pThis = reinterpret_cast<CMainWindow*>(pCreateStruct->lpCreateParams);
		::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
	}

	pThis = reinterpret_cast<CMainWindow*>(::GetWindowLongPtr(hWnd, GWLP_USERDATA));
	if (pThis != nullptr)
	{
		return pThis->handleMessage(hWnd, uMsg, wParam, lParam);
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/* メッセージ処理 */
LRESULT CMainWindow::handleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		return onCreate(hWnd);
	case WM_DESTROY:
		return onDestroy();
	case WM_CLOSE:
		return onClose();
	case WM_PAINT:
		return onPaint();
	case WM_ERASEBKGND:
		return 1;
	case WM_KEYDOWN:
		return onKeyDown(wParam, lParam);
	case WM_KEYUP:
		return onKeyUp(wParam, lParam);
	case WM_COMMAND:
		return onCommand(wParam, lParam);
	case WM_TIMER:
		return onTimer(wParam);
	case WM_MOUSEMOVE:
		return onMouseMove(wParam, lParam);
	case WM_MOUSEWHEEL:
		return onMouseWheel(wParam, lParam);
	case WM_LBUTTONDOWN:
		return onLButtonDown(wParam, lParam);
	case WM_LBUTTONUP:
		return onLButtonUp(wParam, lParam);
	case WM_RBUTTONUP:
		return onRButtonUp(wParam, lParam);
	case WM_MBUTTONUP:
		return onMButtonUp(wParam, lParam);
	default:
		break;
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/* WM_CREATE */
LRESULT CMainWindow::onCreate(HWND hWnd)
{
	m_hWnd = hWnd;

	initialiseMenuBar();
	updateCheckStatusOfSyncImageOption();

	if (!rlyeh::InitialisePlayerSetting())
	{
		showErrorMessageBox(L"Failed to read setting file.");
	}

	if (!m_d3d11Core.initialise(m_hWnd, true))
	{
		showErrorMessageBox(L"Failed to initialise DirectX11 backend.");
		return 0;
	}

	m_d3d11Renderer2d.initialise(m_d3d11Core.getD3d11Device(), m_d3d11Core.getD3D11DeviceContext());
	if (!m_d3d11Renderer2d.hasBeenInitialised())
	{
		showErrorMessageBox(L"Failed to initialise DirectX11 renderer.");
	}

	m_pD3d11SpinePlayer = std::make_unique<CD3D11SpinePlayer>(m_d3d11Core.getD3d11Device(), m_hWnd);
	CD3D11SpineTextureLoader::SetFormatAsBgraOnLoading(true);

	static constexpr wchar_t s_defualtFontFilePath[] = L"C:\\Windows\\Fonts\\yumindb.ttf";

	m_pSceneTextWriter = std::make_unique<CD2TextWriter>(m_d3d11Core.getD2Factory(), m_d3d11Core.getD2DeviceContext());
	m_pSceneTextWriter->onDpiChanged(::GetDpiForWindow(m_hWnd));
	m_pSceneTextWriter->setupOutLinedDrawing(s_defualtFontFilePath);

	m_pHelpTextWriter = std::make_unique<CD2TextWriter>(m_d3d11Core.getD2Factory(), m_d3d11Core.getD2DeviceContext());
	m_pHelpTextWriter->setupOutLinedDrawing(s_defualtFontFilePath, false, false, m_pSceneTextWriter->getFontSize() / 2.f);
	m_pHelpTextWriter->onDpiChanged(::GetDpiForWindow(m_hWnd));
	
	recreateHelpTextTexture();

	return 0;
}
/* WM_DESTROY */
LRESULT CMainWindow::onDestroy()
{
	::PostQuitMessage(0);

	return 0;
}
/* WM_CLOSE */
LRESULT CMainWindow::onClose()
{
	::DestroyWindow(m_hWnd);
	::UnregisterClassW(m_className, m_hInstance);

	return 0;
}
/* WM_PAINT */
LRESULT CMainWindow::onPaint()
{
	PAINTSTRUCT ps;
	HDC hdc = ::BeginPaint(m_hWnd, &ps);

	::EndPaint(m_hWnd, &ps);

	return 0;
}
/* WM_SIZE */
LRESULT CMainWindow::onSize(WPARAM wParam, LPARAM lParam)
{
	return 0;
}
/* WM_KEYDOWN */
LRESULT CMainWindow::onKeyDown(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_RIGHT:
		if (!hasReachedLastScene())
		{
			shiftScene(true);
		}
		break;
	case VK_LEFT:
		shiftScene(false);
		break;
	default:

		break;
	}

	return 0;
}
/* WM_KEYUP */
LRESULT CMainWindow::onKeyUp(WPARAM wParam, LPARAM lParam)
{
	switch (wParam)
	{
	case VK_ESCAPE:
		::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
		break;
	case VK_UP:
		keyUpOnForeFile();
		break;
	case VK_DOWN:
		keyUpOnNextFile();
		break;
	case 'C':
		if (m_pSceneTextWriter != nullptr)
		{
			m_pSceneTextWriter->toggleTextColour();
			recreateSceneTextTexture();
		}

		if (m_pHelpTextWriter != nullptr)
		{
			m_pHelpTextWriter->toggleTextColour();
			recreateHelpTextTexture();
		}
		break;
	case 'H':
		m_sceneState.isHelpTextShown ^= true;
		break;
	case 'T':
		m_sceneState.isSceneTextHidden ^= true;
		break;
	default:
		break;
	}
	return 0;
}
/* WM_COMMAND */
LRESULT CMainWindow::onCommand(WPARAM wParam, LPARAM lParam)
{
	const int msgId = LOWORD(wParam);
	const int msgSource = LOWORD(lParam);
	if (msgSource == 0)
	{
		/* Menus */
		switch (msgId)
		{
		case Menu::kOpenFile:
			menuOnOpenFile();
			break;
		case Menu::kVoiceSetting:
			menuOnVoiceSetting();
			break;
		case Menu::kFontSetting:
			menuOnFontSetting();
			break;
		case Menu::kShowHelp:
			menuOnShowHelp();
			break;
		case Menu::kSyncImage:
			menuOnSyncImage();
			break;
		default:
			break;
		}
	}
	else
	{
		/* Controls */
	}

	return 0;
}
/* WM_TIMER */
LRESULT CMainWindow::onTimer(WPARAM wParam)
{
	return 0;
}
/* WM_MOUSEMOVE */
LRESULT CMainWindow::onMouseMove(WPARAM wParam, LPARAM lParam)
{
	const WORD pressedKeys = LOWORD(wParam);
	if (pressedKeys == MK_LBUTTON)
	{
		if (m_mouseState.wasLeftCombined)return 0;

		POINT pt{};
		::GetCursorPos(&pt);
		if (m_mouseState.wasLeftDragged)
		{
			const int deltaX = pt.x - m_mouseState.lastCursorPos.x;
			const int deltaY = pt.y - m_mouseState.lastCursorPos.y;
			m_pD3d11SpinePlayer->addOffset(-deltaX, -deltaY);
		}

		m_mouseState.lastCursorPos = pt;
		m_mouseState.wasLeftDragged = true;
	}
	else if ((pressedKeys & MK_LBUTTON) && (pressedKeys & MK_RBUTTON))
	{
		if (m_mouseState.wasLeftCombined || m_mouseState.wasRightCombined)return 0;

		POINT pt{};
		::GetCursorPos(&pt);
		if (m_mouseState.wasRightDragged)
		{
			int deltaX = pt.x - m_mouseState.lastCursorPos.x;
			int deltaY = pt.y - m_mouseState.lastCursorPos.y;

			RECT windowRect{};
			::GetWindowRect(m_hWnd, &windowRect);
			::SetWindowPos(m_hWnd, nullptr, windowRect.left + deltaX, windowRect.top + deltaY, 0, 0, SWP_NOSIZE);
		}

		m_mouseState.lastCursorPos = pt;
		m_mouseState.wasRightDragged = true;
	}

	return 0;
}
/* WM_MOUSEWHEEL */
LRESULT CMainWindow::onMouseWheel(WPARAM wParam, LPARAM lParam)
{
	const short scroll = -static_cast<short>(HIWORD(wParam)) / WHEEL_DELTA;
	const WORD pressedKeys = LOWORD(wParam);
	if (pressedKeys == MK_LBUTTON)
	{
		static constexpr float kTimeScaleDelta = 0.05f;
		const float scrollSign = scroll > 0 ? 1.f : -1.f;

		float timeScale = m_pD3d11SpinePlayer->getTimeScale() + kTimeScaleDelta * scrollSign;
		timeScale = (std::max)(timeScale, 0.f);
		m_pD3d11SpinePlayer->setTimeScale(timeScale);

		m_mouseState.wasLeftCombined = true;
	}
	else if (pressedKeys == MK_RBUTTON)
	{
		shiftScene(scroll > 0);

		m_mouseState.wasRightCombined = true;
	}
	else
	{
		if (m_pD3d11SpinePlayer->hasSpineBeenLoaded())
		{
			static constexpr float kScaleDelta = 0.025f;
			static constexpr float kMinScale = 0.15f;
			const float scrollSign = scroll > 0 ? 1.f : -1.f;

			float skeletonScale = m_pD3d11SpinePlayer->getSkeletonScale() + kScaleDelta * scrollSign;
			skeletonScale = (std::max)(kMinScale, skeletonScale);
			m_pD3d11SpinePlayer->setSkeletonScale(skeletonScale);

			const bool isWindowToBeResized = !(pressedKeys & MK_CONTROL);
			if (isWindowToBeResized)
			{
				float canvasScale = m_pD3d11SpinePlayer->getCanvasScale() + kScaleDelta * scrollSign;
				canvasScale = (std::max)(kMinScale, canvasScale);
				m_pD3d11SpinePlayer->setCanvasScale(canvasScale);

				resizeWindow();
			}
		}
	}

	return 0;
}
/* WM_LBUTTONDOWN */
LRESULT CMainWindow::onLButtonDown(WPARAM wParam, LPARAM lParam)
{
	::GetCursorPos(&m_mouseState.lastCursorPos);

	m_mouseState.wasLeftPressed = true;

	return 0;
}
/* WM_LBUTTONUP */
LRESULT CMainWindow::onLButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_mouseState.wasLeftCombined || m_mouseState.wasLeftDragged)
	{
		m_mouseState.wasLeftDragged = false;
		m_mouseState.wasLeftCombined = false;
		m_mouseState.wasLeftPressed = false;

		return 0;
	}

	WORD pressedKeys = LOWORD(wParam);
	if (pressedKeys == 0 && m_mouseState.wasLeftPressed)
	{
		POINT pt{};
		::GetCursorPos(&pt);
		const int deltaX = m_mouseState.lastCursorPos.x - pt.x;
		const int deltaY = m_mouseState.lastCursorPos.y - pt.y;

		if (deltaX == 0 && deltaY == 0)
		{
			if (!m_sceneState.isImageSynced)
			{
				++m_nLastAnimationIndex;
				if (m_nLastAnimationIndex >= m_animationNames.size())m_nLastAnimationIndex = 0;

				const std::string& animationName = m_animationNames[m_nLastAnimationIndex];
				m_pD3d11SpinePlayer->setAnimationByName(animationName.c_str());
			}
		}
	}

	m_mouseState.wasLeftPressed = false;

	return 0;
}
/* WM_RBUTTONUP */
LRESULT CMainWindow::onRButtonUp(WPARAM wParam, LPARAM lParam)
{
	if (m_mouseState.wasRightCombined || m_mouseState.wasRightDragged)
	{
		m_mouseState.wasRightCombined = false;
		m_mouseState.wasRightDragged = false;

		return 0;
	}

	WORD pressedKeys = LOWORD(wParam);
	if (pressedKeys == 0)
	{
		if (!m_labelData.empty())
		{
			window_menu::CContextMenu contextMenu;
			for (size_t i = 0; i < m_labelData.size(); ++i)
			{
				const auto& lebalDatum = m_labelData[i];
				contextMenu.addMenuItem({ i + 1, lebalDatum.caption.c_str() });
			}

			BOOL menuIndex = contextMenu.display(m_hWnd);
			if (menuIndex > 0)
			{
				const size_t nLabelIndex = static_cast<size_t>(menuIndex - 1);
				m_nSceneIndex = m_labelData[nLabelIndex].nSceneIndex;
				prepareScene();
				m_spineClock.restart();
				m_sceneTextClock.restart();
			}
		}
	}

	return 0;
}
/* WM_MBUTTONUP */
LRESULT CMainWindow::onMButtonUp(WPARAM wParam, LPARAM lParam)
{
	WORD pressedKeys = LOWORD(wParam);
	if (pressedKeys == 0)
	{
		if (m_pD3d11SpinePlayer->hasSpineBeenLoaded())
		{
			resetSpinePlayerScale();
		}
	}
	else if (pressedKeys == MK_RBUTTON)
	{
		toggleWindowBorderStyle();

		m_mouseState.wasRightCombined = true;
	}

	return 0;
}

void CMainWindow::tick() noexcept
{
	if (m_pD3d11SpinePlayer != nullptr && m_pD3d11SpinePlayer->hasSpineBeenLoaded())
	{
		const float fDelta = m_spineClock.getElapsedTime();
		m_pD3d11SpinePlayer->update(fDelta);

		m_d3d11Core.bindRenderTarget(nullptr);
		m_d3d11Core.clear();
		m_d3d11Renderer2d.setupRenderState();

		RECT clientRect;
		::GetClientRect(m_hWnd, &clientRect);
		const UINT clientWidth = clientRect.right - clientRect.left;
		const UINT clientHeight = clientRect.bottom - clientRect.top;

		float fScale = 1.f, fOffsetX = 0.f, fOffsetY = 0.f;
		m_pD3d11SpinePlayer->calculateTransformValues(clientWidth, clientHeight, fScale, fOffsetX, fOffsetY);
		m_pD3d11SpinePlayer->draw(fScale, fOffsetX, fOffsetY);

		if (m_sceneState.isHelpTextShown)
		{
			const FLOAT helpTextOffsetY = static_cast<FLOAT>(clientHeight - m_helpTextureSize.height);
			m_d3d11Renderer2d.drawTexture(m_pHelpTextResourceView, 0.f, helpTextOffsetY);
		}

		if (!m_sceneState.isSceneTextHidden)
		{
			m_d3d11Renderer2d.drawTexture(m_pSceneTextResourceView);
		}

		m_spineClock.restart();

		m_d3d11Core.display();

		checkSceneTextTimer();
	}
}
/* 操作欄作成 */
void CMainWindow::initialiseMenuBar() noexcept
{
	if (::IsMenu(m_hMenuBar))return;

	HMENU hMenu = window_menu::MenuBuilder(
		{
			{ 0, L"File", window_menu::MenuBuilder(
				{
					{Menu::kOpenFile, L"Open"}
				}).get()
			},
			{ 0, L"Setting", window_menu::MenuBuilder(
				{
					{Menu::kVoiceSetting, L"Voice"},
					{Menu::kFontSetting, L"Font"}
				}).get(),
			},
			{ 0, L"Option", window_menu::MenuBuilder(
				{
					{Menu::kShowHelp, L"Show help"},
					{Menu::kSyncImage, L"Sync image"}
				}).get(),
			}
		}).get();

	if (!::IsMenu(hMenu) || ::SetMenu(m_hWnd, hMenu) == 0)
	{
		showErrorMessageBox(L"Failed to create menu; code: %ul", ::GetLastError());

		if (::IsMenu(hMenu))::DestroyMenu(hMenu);
	}
	else
	{
		m_hMenuBar = hMenu;
	}
}

void CMainWindow::showErrorMessageBox(const wchar_t* format, ...) const noexcept
{
	wchar_t messageBuffer[512]{};
	constexpr size_t kBufferCapacity = sizeof(messageBuffer) / sizeof(wchar_t) - 1;

	va_list args;
	va_start(args, format);
	vswprintf_s(messageBuffer, kBufferCapacity, format, args);
	va_end(args);

	/* Might be busy in handling WM_PAINT. */
	if (m_hWnd != nullptr)::ValidateRect(m_hWnd, nullptr);
	::MessageBoxW(m_hWnd, messageBuffer, L"Error", MB_ICONERROR);
	if (m_hWnd != nullptr)::InvalidateRect(m_hWnd, nullptr, FALSE);
}

void CMainWindow::resizeWindow() noexcept
{
	const D2D_POINT_2F fBaseSize = m_pD3d11SpinePlayer->getBaseSize();
	const float fScale = m_pD3d11SpinePlayer->getCanvasScale();

	RECT rect;
	::GetClientRect(m_hWnd, &rect);
	int clientWidth = static_cast<int>(fBaseSize.x * fScale);
	int clientHeight = static_cast<int>(fBaseSize.y * fScale);

	HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
	if (hMonitor != nullptr)
	{
		MONITORINFO monitorInfo{ sizeof(MONITORINFO) };
		BOOL iRet = ::GetMonitorInfoW(hMonitor, &monitorInfo);
		if (iRet)
		{
			const int monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
			const int monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;

			/* No need to resize window, nor to recreate render-texture(s). */
			if (clientWidth == monitorWidth && clientHeight == monitorHeight)return;

			clientWidth = (std::min)(clientWidth, monitorWidth);
			clientHeight = (std::min)(clientHeight, monitorHeight);
		}
	}

	rect.right = clientWidth + rect.left;
	rect.bottom = clientHeight + rect.top;

	LONG lStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);
	const bool isWindowBarHidden = !((lStyle & WS_CAPTION) && (lStyle & WS_SYSMENU));

	::AdjustWindowRect(&rect, lStyle, isWindowBarHidden ? FALSE : TRUE);
	::SetWindowPos(m_hWnd, HWND_TOP, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, SWP_NOMOVE | SWP_NOZORDER);

	m_d3d11Core.onResize();
	m_d3d11Renderer2d.updateViewport(static_cast<FLOAT>(clientWidth), static_cast<FLOAT>(clientHeight));

	recreateSceneTextTexture();
}

void CMainWindow::changeWindowTitle(const wchar_t* windowTitle) noexcept
{
	const wchar_t* pTruncatedWindowTitle = windowTitle;
	if (pTruncatedWindowTitle != nullptr)
	{
		for (;;)
		{
			const wchar_t* pPos = ::wcspbrk(pTruncatedWindowTitle, L"\\/");
			if (pPos == nullptr)break;
			pTruncatedWindowTitle = pPos + 1;
		}
	}

	::SetWindowTextW(m_hWnd, pTruncatedWindowTitle == nullptr ? m_defaultWindowName : pTruncatedWindowTitle);
}

void CMainWindow::toggleWindowBorderStyle() noexcept
{
	if (!m_pD3d11SpinePlayer->hasSpineBeenLoaded())return;

	RECT rect;
	::GetWindowRect(m_hWnd, &rect);
	const LONG lStyle = ::GetWindowLong(m_hWnd, GWL_STYLE);

	m_windowStyle.isBorderless ^= true;

	if (m_windowStyle.isBorderless)
	{
		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle & ~WS_CAPTION & ~WS_SYSMENU & ~WS_THICKFRAME);
		::SetWindowPos(m_hWnd, nullptr, 0, 0, rect.right - rect.left, rect.bottom - rect.top, SWP_NOZORDER);
		::SetMenu(m_hWnd, nullptr);
	}
	else
	{
		::SetWindowLong(m_hWnd, GWL_STYLE, lStyle | WS_CAPTION | WS_SYSMENU);
		::SetMenu(m_hWnd, m_hMenuBar);
	}

	resizeWindow();
}

void CMainWindow::menuOnOpenFile()
{
	const std::wstring_view& scriptFileFilter = rlyeh::GetScriptFileFilter();
	std::wstring selectedFilePath = win_dialogue::SelectOpenFile(L"Script file", scriptFileFilter.data(), L"Select *.book file under advscene/scenarioexcel/character/XXXXX folder", m_hWnd);
	if (selectedFilePath.empty())return;

	bool bRet = loadScenario(selectedFilePath);
	if (bRet)
	{
		m_scriptFilePaths.clear();
		m_nScriptPathIndex = 0;
		rlyeh::BuildScriptFilePathList(selectedFilePath, m_scriptFilePaths, m_nScriptPathIndex);
	}
}

void CMainWindow::menuOnVoiceSetting() noexcept
{
	if (m_mediaSettingDialogue.getHwnd() == nullptr)
	{
		HWND hWnd = m_mediaSettingDialogue.open(m_hInstance, m_hWnd, &m_voicePlayer, L"Voice");
		::ShowWindow(hWnd, SW_SHOWNORMAL);
	}
	else
	{
		::SetFocus(m_mediaSettingDialogue.getHwnd());
	}
}

void CMainWindow::menuOnFontSetting() noexcept
{
	if (m_fontSettingDialogue.getHwnd() == nullptr)
	{
		static const auto FontChangeCallback = [](void* pUserDatum, CFontSettingDialogue::FontCallbackDatum* pFontCallbackDatum)
			-> void
			{
				CMainWindow* pThis = static_cast<CMainWindow*>(pUserDatum);
				if (pThis != nullptr)
				{
					pThis->m_pHelpTextWriter->setupOutLinedDrawing(pFontCallbackDatum->fontFilePath, false, false, pFontCallbackDatum->fontSize / 2.f);

					pThis->recreateSceneTextTexture();
					pThis->recreateHelpTextTexture();
				}
			};

		HWND hWnd = m_fontSettingDialogue.open(m_hInstance, m_hWnd, L"Font", m_pSceneTextWriter.get(), FontChangeCallback, this);
		::ShowWindow(hWnd, SW_SHOWNORMAL);
	}
	else
	{
		::SetFocus(m_fontSettingDialogue.getHwnd());
	}
}

void CMainWindow::menuOnShowHelp() noexcept
{
	m_sceneState.isHelpTextShown ^= true;
	window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kOption), Menu::kShowHelp, m_sceneState.isHelpTextShown);
}

void CMainWindow::menuOnSyncImage() noexcept
{
	m_sceneState.isImageSynced ^= true;
	updateCheckStatusOfSyncImageOption();
	if (m_sceneState.isImageSynced)
	{
		prepareAnimation();
	}
}

void CMainWindow::keyUpOnNextFile() noexcept
{
	if (m_scriptFilePaths.empty())return;

	++m_nScriptPathIndex;
	if (m_nScriptPathIndex >= m_scriptFilePaths.size())
	{
		m_nScriptPathIndex = 0;
	}

	loadScenario(m_scriptFilePaths[m_nScriptPathIndex]);
}

void CMainWindow::keyUpOnForeFile() noexcept
{
	if (m_scriptFilePaths.empty())return;

	--m_nScriptPathIndex;
	if (m_nScriptPathIndex >= m_scriptFilePaths.size())
	{
		m_nScriptPathIndex = m_scriptFilePaths.size() - 1;
	}

	loadScenario(m_scriptFilePaths[m_nScriptPathIndex]);
}

void CMainWindow::updateCheckStatusOfSyncImageOption() const noexcept
{
	window_menu::SetMenuCheckState(window_menu::GetMenuInBar(m_hWnd, MenuBar::kOption), Menu::kSyncImage, m_sceneState.isImageSynced);
}

bool CMainWindow::loadScenario(const std::wstring& scriptFilePath)
{
	clearScenerioData();

	std::wstring animationFolderPath;
	bool bRet = rlyeh::ReadScenarioScript(scriptFilePath, m_textData, m_animationNames, animationFolderPath, m_sceneData, m_labelData);
	if (!bRet)return false;
	
	std::wstring atlasFilePath;
	std::wstring skelFilePath;
	rlyeh::FindSceneAnimationFiles(animationFolderPath, atlasFilePath, skelFilePath);
	if (atlasFilePath.empty() || skelFilePath.empty())return false;

	std::string atlasFile = win_filesystem::LoadFileAsString(atlasFilePath.data());
	std::string skelFile = win_filesystem::LoadFileAsString(skelFilePath.data());

	static const auto ExtractParentDirectoryAndNarrowIt = [](const std::wstring& filePath)
		->std::string
		{
			size_t nPos1 = filePath.find_last_of(L"\\/");
			if (nPos1 == std::wstring::npos)nPos1 = 0;
			else ++nPos1;

			return win_text::NarrowUtf8(&filePath[0], nPos1);
		};
	std::string textureDirectory = ExtractParentDirectoryAndNarrowIt(atlasFilePath);

	using namespace spine_file_verifier;
	SkeletonMetadata skeletonMetaData = VerifySkeletonFileData(reinterpret_cast<const unsigned char*>(skelFile.data()), skelFile.size());
	if (skeletonMetaData.skeletonFormat == SkeletonFormat::Neither) return false;
	if (!skeletonMetaData.version.starts_with("4.2")) return false;
	const bool isBinarySkel = skeletonMetaData.skeletonFormat == SkeletonFormat::Binary;

	m_pD3d11SpinePlayer->clearDrawables();
	const bool hasLoaded = m_pD3d11SpinePlayer->loadSpineFromMemory(
		atlasFile.data(), atlasFile.size(),
		textureDirectory.data(),
		skelFile.data(), skelFile.size(),
		isBinarySkel, &m_d3d11Renderer2d);
	if (!hasLoaded)return false;

	static constexpr auto SpineLeaveOutCallback = [](const char* slotName, size_t slotNameLength, [[maybe_unused]] void* pUserDatum)
		-> bool
		{
			static constexpr std::string_view s_slotNamesToExclude[] =
			{
				"Mask", "WhiteSpot"
			};

			const char* pNameEnd = slotName + slotNameLength;
			bool isToBeLeftOut = false;
			for (const auto& slotNameToExclude : s_slotNamesToExclude)
			{
				const char* pFilterStart = slotNameToExclude.data();
				const char* pFilterEnd = slotNameToExclude.data() + slotNameToExclude.size();
				isToBeLeftOut |= std::search(slotName, pNameEnd, pFilterStart, pFilterEnd) != pNameEnd;
			}

			return isToBeLeftOut;
		};
	m_pD3d11SpinePlayer->setSlotExcludeCallback(SpineLeaveOutCallback, this);
	m_pD3d11SpinePlayer->setPhysicsAll(CD3D11SpineDrawable::Physics::None);
	m_pD3d11SpinePlayer->resetBaseSize();

	changeWindowTitle(scriptFilePath.data());
	resetSpinePlayerScale();
	prepareScene();
	m_spineClock.restart();
	m_sceneTextClock.restart();

	return true;
}

void CMainWindow::clearScenerioData() noexcept
{
	m_textData.clear();

	m_animationNames.clear();
	m_nLastAnimationIndex = static_cast<size_t>(-1LL);

	m_sceneData.clear();
	m_nSceneIndex = 0;

	m_labelData.clear();

	m_formattedSceneText.clear();
}

void CMainWindow::resetSpinePlayerScale()
{
	m_pD3d11SpinePlayer->resetScale();
	m_pD3d11SpinePlayer->setSkeletonScale(m_pD3d11SpinePlayer->getSkeletonScale() + 0.05f);
	resizeWindow();
}

void CMainWindow::recreateSceneTextTexture() noexcept
{
	if (m_formattedSceneText.empty())return;

	RECT rc;
	::GetClientRect(m_hWnd, &rc);
	const float wrapWidth = static_cast<float>(rc.right - rc.left);

	m_pSceneTextResourceView.Release();
	drawTextOnBitapCompatibleWithDirect3D(m_pSceneTextWriter.get(), m_formattedSceneText.c_str(), m_formattedSceneText.size(), &m_pSceneTextResourceView, wrapWidth);
}

void CMainWindow::recreateHelpTextTexture() noexcept
{
	static constexpr wchar_t s_helpText[] =
	{
		L"[H] Hide/show help\n"
		L"[T] Hide/show scene text\n"
		L"[C] Toggle text colour\n"
		L"[Wheel scroll] Scale up/down\n"
		L"[L-drag] Move view-point\n"
		L"[L-pressed + scroll] Speed up/down the animation\n"
		L"[M-click] Reset speed, scale and view-point\n"
		L"[R-click] Show context menu to jump scene\n"
		L"[R-pressed + M-click] Hide/show the border of window\n"
		L"[R-pressed + L-drag] Move borderless window\n"
		L"[←|→; R-pressed + scroll] Rewind/fast-forward the scene text\n"
		L"[↑|↓] Open the previous/next folder\n"
	};
	static constexpr size_t kHelpTextLength = sizeof(s_helpText) / sizeof(s_helpText[0]) - 1;

	m_pHelpTextResourceView.Release();
	drawTextOnBitapCompatibleWithDirect3D(m_pHelpTextWriter.get(), s_helpText, kHelpTextLength, &m_pHelpTextResourceView);

	if (m_pHelpTextResourceView != nullptr)
	{
		CComPtr<ID3D11Resource> pD3D11Resource;
		m_pHelpTextResourceView->GetResource(&pD3D11Resource);
		if (pD3D11Resource != nullptr)
		{
			CComPtr<ID3D11Texture2D> pD3D11Texture2D;
			pD3D11Resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pD3D11Texture2D);
			if (pD3D11Texture2D != nullptr)
			{
				D3D11_TEXTURE2D_DESC d3d11Texture2dDesc;
				pD3D11Texture2D->GetDesc(&d3d11Texture2dDesc);

				m_helpTextureSize.width = d3d11Texture2dDesc.Width;
				m_helpTextureSize.height = d3d11Texture2dDesc.Height;
			}
		}
	}
}

void CMainWindow::drawTextOnBitapCompatibleWithDirect3D(CD2TextWriter* pTextWriter, const wchar_t* text, size_t textLength, ID3D11ShaderResourceView** targetResourceView, float wrapWidth) noexcept
{
	D2D1_SIZE_F textBounds = pTextWriter->calculateTextBounds(text, textLength, wrapWidth);
	const D2D_POINT_2F textOffset = { pTextWriter->getThickness(), pTextWriter->getThickness() };
	textBounds.width += textOffset.x * 2.f;
	textBounds.height += textOffset.y * 2.f;
	const D2D1_SIZE_U bitmapSize{ static_cast<UINT>(::ceilf(textBounds.width)), static_cast<UINT>(::ceilf(textBounds.height)) };

	CComPtr<ID2D1Bitmap1> pTargetBitmap;
	m_d3d11Core.createD3CompatibleD2Bitmap(bitmapSize, &pTargetBitmap, targetResourceView);
	if (pTargetBitmap != nullptr)
	{
		CComPtr<ID2D1Image> pPreviousRendererTarget;
		m_d3d11Core.getD2DeviceContext()->GetTarget(&pPreviousRendererTarget);

		m_d3d11Core.getD2DeviceContext()->SetTarget(pTargetBitmap);
		pTextWriter->outLinedDraw(text, textLength, wrapWidth, textOffset);
		m_d3d11Core.getD2DeviceContext()->SetTarget(pPreviousRendererTarget);
	}
}

void CMainWindow::shiftScene(bool forward) noexcept
{
	if (forward)
	{
		++m_nSceneIndex;
		if (m_nSceneIndex >= m_sceneData.size())m_nSceneIndex = 0;
	}
	else
	{
		--m_nSceneIndex;
		if (m_nSceneIndex >= m_sceneData.size())m_nSceneIndex = m_sceneData.size() - 1;
	}

	prepareScene();
}

void CMainWindow::prepareScene() noexcept
{
	prepareText();
	prepareAnimation();
}

bool CMainWindow::hasReachedLastScene() const noexcept
{
	return m_nSceneIndex == m_sceneData.size() - 1;
}

void CMainWindow::prepareText()
{
	if (m_nSceneIndex >= m_sceneData.size())return;

	const size_t nTextIndex = m_sceneData[m_nSceneIndex].nTextIndex;
	if (nTextIndex >= m_textData.size())return;

	wchar_t indexBuffer[32]{};
	int nWritten = swprintf_s(indexBuffer, L"%zu/%zu", nTextIndex + 1, m_textData.size());

	const auto& t = m_textData[nTextIndex];
	if (t.characterName.empty())
	{
		m_formattedSceneText.clear();
	}
	else
	{
		m_formattedSceneText.assign(t.characterName).append(L":");
	}
	m_formattedSceneText.append(L" \n").append(t.sceneText);
	if (m_formattedSceneText.back() != L'\n')m_formattedSceneText.push_back(L'\n');
	if(nWritten != -1)m_formattedSceneText.append(indexBuffer, nWritten);

	recreateSceneTextTexture();

	if (!t.voiceFilePath.empty())
	{
		m_voicePlayer.play(t.voiceFilePath.data());
	}
}

void CMainWindow::prepareAnimation() noexcept
{
	if (!m_sceneState.isImageSynced)return;

	if (m_nSceneIndex >= m_sceneData.size())return;

	const size_t nAnimationIndex = m_sceneData[m_nSceneIndex].nAnimationIndex;
	if (nAnimationIndex >= m_animationNames.size())return;

	if (nAnimationIndex != m_nLastAnimationIndex)
	{
		const std::string& animationName = m_animationNames[nAnimationIndex];
		m_pD3d11SpinePlayer->setAnimationByName(animationName.c_str());

		m_nLastAnimationIndex = nAnimationIndex;
	}
}

void CMainWindow::checkSceneTextTimer() noexcept
{
	float fElapsed = m_sceneTextClock.getElapsedTime();
	if (::isgreaterequal(fElapsed, 3.f))
	{
		if (m_voicePlayer.isEnded())
		{
			m_sceneTextClock.restart();
			if (!hasReachedLastScene())
			{
				shiftScene(true);
			}
		}
	}
}
