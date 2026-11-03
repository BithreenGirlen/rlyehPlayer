#ifndef MAIN_WINDOW_H_
#define MAIN_WINDOW_H_

#include <Windows.h>
#include <atlbase.h>

#include <memory>

#include "d3d11_core.h"
#include "d2_text_writer.h"
#include "d3d11_renderer2d.h"
#include "d3d11_spine/d3d11_spine_player.h"
#include "mf_media_player.h"
#include "win_clock.h"
#include "adv.h"
#include "native-ui/media_setting_dialogue.h"
#include "native-ui/font_setting_dialogue.h"

class CMainWindow
{
public:
	CMainWindow() = default;
	~CMainWindow() = default;

	bool create(HINSTANCE hInstance, const wchar_t* windowName, HICON hIcon = nullptr);
	int messageLoop();

	HWND getHwnd()const noexcept { return m_hWnd; }
private:
	const wchar_t* m_className = L"rlyer player window";
	const wchar_t* m_defaultWindowName = L"rlyer player";

	HINSTANCE m_hInstance = nullptr;
	HWND m_hWnd = nullptr;

	static LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT handleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT onCreate(HWND hWnd);
	LRESULT onDestroy();
	LRESULT onClose();
	LRESULT onPaint();
	LRESULT onSize(WPARAM wParam, LPARAM lParam);
	LRESULT onKeyDown(WPARAM wParam, LPARAM lParam);
	LRESULT onKeyUp(WPARAM wParam, LPARAM lParam);
	LRESULT onCommand(WPARAM wParam, LPARAM lParam);
	LRESULT onTimer(WPARAM wParam);
	LRESULT onMouseMove(WPARAM wParam, LPARAM lParam);
	LRESULT onMouseWheel(WPARAM wParam, LPARAM lParam);
	LRESULT onLButtonDown(WPARAM wParam, LPARAM lParam);
	LRESULT onLButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT onRButtonUp(WPARAM wParam, LPARAM lParam);
	LRESULT onMButtonUp(WPARAM wParam, LPARAM lParam);

	struct Menu
	{
		enum
		{
			kOpenFile = 1,
			kVoiceSetting, kFontSetting,
			kShowHelp, kSyncImage
		};
	};
	struct MenuBar { enum { kFile, kSetting, kOption }; };

	struct WindowStyle
	{
		bool isBorderless = false;
	};

	struct MouseState
	{
		/// @brief 左釦押下を要する操作を行ったか。
		bool wasLeftCombined = false;
		bool wasLeftPressed = false;
		bool wasLeftDragged = false;
		/// @brief 右釦押下を要する操作を行ったか。
		bool wasRightCombined = false;
		bool wasRightDragged = false;

		POINT lastCursorPos{};
	};

	struct SceneState
	{
		bool isSceneTextHidden = false;
		bool isHelpTextShown = false;
		bool isImageSynced = true;
	};

	WindowStyle m_windowStyle;
	MouseState m_mouseState;
	SceneState m_sceneState;

	HMENU m_hMenuBar = nullptr;

	void tick() noexcept;

	void initialiseMenuBar() noexcept;
	void showErrorMessageBox(const wchar_t* format, ...) const noexcept;

	void resizeWindow() noexcept;
	void changeWindowTitle(const wchar_t* windowTitle) noexcept;
	void toggleWindowBorderStyle() noexcept;

	void menuOnOpenFile();

	void menuOnVoiceSetting() noexcept;
	void menuOnFontSetting() noexcept;

	void menuOnShowHelp() noexcept;
	void menuOnSyncImage() noexcept;

	void keyUpOnNextFile() noexcept;
	void keyUpOnForeFile() noexcept;

	void updateCheckStatusOfSyncImageOption() const noexcept;

	CMediaSettingDialogue m_mediaSettingDialogue;
	CFontSettingDialogue m_fontSettingDialogue;

	CD3D11Core m_d3d11Core;
	d3d11_renderer2d::CD3D11Renderer2D m_d3d11Renderer2d;
	std::unique_ptr<CD3D11SpinePlayer> m_pD3d11SpinePlayer;
	std::unique_ptr<CD2TextWriter> m_pSceneTextWriter;
	std::unique_ptr<CD2TextWriter> m_pHelpTextWriter;
	CMfMediaPlayer m_voicePlayer;

	CWinClock m_spineClock;
	CWinClock m_sceneTextClock;

	std::vector<std::wstring> m_scriptFilePaths;
	size_t m_nScriptPathIndex = 0;

	std::vector<adv::TextDatum> m_textData;
	std::vector<std::string> m_animationNames;
	size_t m_nLastAnimationIndex = static_cast<size_t>(-1LL);
	std::vector<adv::SceneDatum> m_sceneData;
	size_t m_nSceneIndex = 0;
	std::vector<adv::LabelDatum> m_labelData;

	std::wstring m_formattedSceneText;

	CComPtr<ID3D11ShaderResourceView> m_pSceneTextResourceView;
	CComPtr<ID3D11ShaderResourceView> m_pHelpTextResourceView;
	D2D1_SIZE_U m_helpTextureSize{};

	bool loadScenario(const std::wstring& scriptFilePath);
	void clearScenerioData() noexcept;

	void resetSpinePlayerScale();

	void recreateSceneTextTexture() noexcept;
	void recreateHelpTextTexture() noexcept;
	void drawTextOnBitapCompatibleWithDirect3D(CD2TextWriter* pTextWriter, const wchar_t* text, size_t textLength, ID3D11ShaderResourceView** targetResourceView, float wrapWidth = 0.f) noexcept;

	void shiftScene(bool forward) noexcept;
	void prepareScene() noexcept;
	bool hasReachedLastScene() const noexcept;

	void prepareText();
	void prepareAnimation() noexcept;

	void checkSceneTextTimer() noexcept;
};

#endif //MAIN_WINDOW_H_