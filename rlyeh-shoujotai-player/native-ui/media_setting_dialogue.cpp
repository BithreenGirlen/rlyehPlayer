

#include "media_setting_dialogue.h"

#include "dialogue_template.h"
#include "../mf_media_player.h"

CMediaSettingDialogue::CMediaSettingDialogue()
{
	int fontHeight = static_cast<int>(Constants::kFontSize * ::GetDpiForSystem() / 96.f);
	m_hFont = ::CreateFontW(fontHeight, 0, 0, 0, FW_REGULAR, FALSE, FALSE, FALSE, EASTEUROPE_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, L"Yu mincho");
}

CMediaSettingDialogue::~CMediaSettingDialogue()
{
	if (m_hFont != nullptr)
	{
		::DeleteObject(m_hFont);
	}
}

HWND CMediaSettingDialogue::open(HINSTANCE hInstance, HWND hWndParent, void* pMediaPlayer, const wchar_t* windowName)
{
	CDialogueTemplate dialogueTemplate;
	dialogueTemplate.setWindowSize(80, 120);

	m_pMediaPlayer = pMediaPlayer;

	return ::CreateDialogIndirectParam(hInstance, (LPCDLGTEMPLATE)dialogueTemplate.generate(windowName), hWndParent, (DLGPROC)DialogProc, (LPARAM)this);
}
/* C CALLBACK */
LRESULT CMediaSettingDialogue::DialogProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (uMsg == WM_INITDIALOG)
	{
		::SetWindowLongPtr(hWnd, DWLP_USER, lParam);
	}

	auto pThis = reinterpret_cast<CMediaSettingDialogue*>(::GetWindowLongPtr(hWnd, DWLP_USER));
	if (pThis != nullptr)
	{
		return pThis->handleMessage(hWnd, uMsg, wParam, lParam);
	}

	return FALSE;
}
/* メッセージ処理 */
LRESULT CMediaSettingDialogue::handleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		return onInit(hWnd);
	case WM_CLOSE:
		return onClose();
	case WM_SIZE:
		return onSize();
	case WM_NOTIFY:
		return onNotify(wParam, lParam);
	case WM_COMMAND:
		return onCommand(wParam, lParam);
	case WM_VSCROLL:
		return onVScroll(wParam, lParam);
	default:
		break;
	}

	return ::DefWindowProcW(hWnd, uMsg, wParam, lParam);
}
/* WM_INITDIALOG */
LRESULT CMediaSettingDialogue::onInit(HWND hWnd)
{
	m_hWnd = hWnd;

	m_volumeIntSlider.create(L"", m_hWnd, reinterpret_cast<HMENU>(Controls::kVolumeSlider), 0, 100, 20, true);
	m_rateFloatSlider.create(L"", m_hWnd, reinterpret_cast<HMENU>(Controls::kRateSlider), 0.5f, 2.5f, 0.1f, 20, true);
	m_volumeStatic.create(L"Volume", m_hWnd);
	m_rateStatic.create(L"Rate", m_hWnd);

	resizeControls();
	setSliderPosition();

	static const auto FontCallback = [](HWND hWnd, LPARAM lParam)
		-> BOOL
		{
			::SendMessage(hWnd, WM_SETFONT, static_cast<WPARAM>(lParam), 0);
			/* TRUE: 続行, FALSE: 終了 */
			return TRUE;
		};

	::EnumChildWindows(m_hWnd, FontCallback, reinterpret_cast<LPARAM>(m_hFont));

	return 0;
}
/* WM_CLOSE */
LRESULT CMediaSettingDialogue::onClose()
{
	::DestroyWindow(m_hWnd);
	m_hWnd = nullptr;

	return 0;
}
/* WM_SIZE */
LRESULT CMediaSettingDialogue::onSize()
{
	resizeControls();

	return 0;
}
/* WM_NOTIFY */
LRESULT CMediaSettingDialogue::onNotify(WPARAM wParam, LPARAM lParam)
{
	LPNMHDR pNmhdr = reinterpret_cast<LPNMHDR>(lParam);
	if (pNmhdr != nullptr)
	{
		if (pNmhdr->code == TTN_NEEDTEXT)
		{
			if (pNmhdr->hwndFrom == m_rateFloatSlider.getToolTipHandle())
			{
				m_rateFloatSlider.onToolTipNeedText(reinterpret_cast<LPTOOLTIPTEXTW>(lParam));
			}
		}
	}
	return 0;
}
/* WM_COMMAND */
LRESULT CMediaSettingDialogue::onCommand(WPARAM wParam, LPARAM lParam)
{
	int id = LOWORD(wParam);
	int msgSource = LOWORD(lParam);
	if (msgSource == 0)
	{
		/* Menus */
	}
	else
	{
		/* Controls */
	}

	return 0;
}
/* WM_VSCROLL */
LRESULT CMediaSettingDialogue::onVScroll(WPARAM wParam, LPARAM lParam)
{
	CMfMediaPlayer* pPlayer = static_cast<CMfMediaPlayer*>(m_pMediaPlayer);
	if (pPlayer != nullptr)
	{
		HANDLE hScroll = reinterpret_cast<HANDLE>(lParam);

		if (hScroll == m_volumeIntSlider.getHwnd())
		{
			double volume = m_volumeIntSlider.getPosition() / 100.0;
			if (volume != pPlayer->getCurrentVolume())
			{
				pPlayer->setCurrentVolume(volume);
			}
		}
		else if (hScroll == m_rateFloatSlider.getHwnd())
		{
			double playbackRate = m_rateFloatSlider.getPosition();
			if (playbackRate != pPlayer->getCurrentRate())
			{
				pPlayer->setCurrentRate(playbackRate);
			}
		}
	}

	return 0;
}

void CMediaSettingDialogue::resizeControls()
{
	RECT rect;
	::GetClientRect(m_hWnd, &rect);
	long clientWidth = rect.right - rect.left;
	long clientHeight = rect.bottom - rect.top;

	long spaceX = clientWidth / 96 * 10;
	long spaceY = clientHeight / 96;

	int fontHeight = static_cast<int>(Constants::kFontSize * ::GetDpiForSystem() / 96.f);

	long x = spaceX;
	long y = spaceY + fontHeight;
	long w = clientWidth / 2 - spaceX * 2;
	long h = clientHeight - spaceY * 2 - fontHeight;
	::MoveWindow(m_volumeIntSlider.getHwnd(), x, y, w, h, TRUE);

	y = spaceY;
	w = Constants::kTextWidth;
	h = fontHeight;
	::MoveWindow(m_volumeStatic.getHwnd(), x, y, w, h, TRUE);

	x = clientWidth / 2 + spaceX;
	y = spaceY + fontHeight;
	w = clientWidth / 2 - spaceX * 2;
	h = clientHeight - spaceY * 2 - fontHeight;
	::MoveWindow(m_rateFloatSlider.getHwnd(), x, y, w, h, TRUE);

	y = spaceY;
	w = Constants::kTextWidth;
	h = fontHeight;
	::MoveWindow(m_rateStatic.getHwnd(), x, y, w, h, TRUE);
}
/* 現在値取得・表示 */
void CMediaSettingDialogue::setSliderPosition()
{
	CMfMediaPlayer* pPlayer = static_cast<CMfMediaPlayer*>(m_pMediaPlayer);
	if (pPlayer != nullptr)
	{
		if (m_volumeIntSlider.getHwnd() != nullptr)
		{
			double dbVolume = pPlayer->getCurrentVolume() * 100.0;
			m_volumeIntSlider.setPosition(static_cast<long long>(dbVolume));
		}

		if (m_rateFloatSlider.getHwnd() != nullptr)
		{
			double dbRate = pPlayer->getCurrentRate();
			m_rateFloatSlider.setPosition(static_cast<float>(dbRate));
		}
	}
}