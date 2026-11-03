
#include "rlyeh.h"

#include "static_string.h"
#include "win_filesystem.h"
#include "win_dialogue.h" /* To show parse error */
#include "win_text.h"
#include "path_utility.h"
#include "text_utility.h"

#include "../deps/nlohmann/json.hpp"

/* 内部用 */
namespace rlyeh
{
	static constexpr const size_t kMaxPathLength = 1024;

	using StaticWString32 = StaticString<wchar_t, 32>;
	using StaticWStringMaxPath = StaticString<wchar_t, kMaxPathLength>;

	static constexpr std::wstring_view s_scriptFileFilter = L"*_R18.book.*";

	/// @brief 素材拡張子設定
	struct PlayerSetting
	{
		StaticWString32 atlasExtension = L".atlas";
		StaticWString32 skelExtension = L".skel";
		StaticWString32 voiceExtension = L".m4a";
	};

	static PlayerSetting g_playerSetting;

	/// @brief 素材拡張子設定ファイル読み込み
	static bool ReadSettingFile(PlayerSetting& playerSetting)
	{
		StaticWStringMaxPath settingFilePath;
		settingFilePath.assign(win_filesystem::GetCurrentProcessPath()).append(std::wstring_view(L"setting.txt"));

		std::string settingFile = win_filesystem::LoadFileAsString(settingFilePath.data());
		if (settingFile.empty())return false;

		try
		{
			nlohmann::json nlJson = nlohmann::json::parse(settingFile);
			const nlohmann::json& jExtension = nlJson.at("extensions");

			static const auto AssignExtension = [&jExtension](const char* fieldName, StaticWString32& dst)
				-> bool
				{
					wchar_t extensionBuffer[32]{};
					static constexpr size_t kBufferSize = std::size(extensionBuffer) - 1;
					
					const std::string* pStr = jExtension.at(fieldName).get<const std::string*>();
					if (pStr == nullptr)return false;

					const int nWritten = win_text::WidenUtf8InBuffer(pStr->c_str(), pStr->size(), extensionBuffer, kBufferSize);
					if (nWritten == 0)return false;

					dst.assign(extensionBuffer, nWritten);

					return true;
				};

			AssignExtension("atlas", playerSetting.atlasExtension);
			AssignExtension("skel", playerSetting.skelExtension);
			AssignExtension("voice", playerSetting.voiceExtension);
		}
		catch (const nlohmann::json::exception& e)
		{
			win_dialogue::ShowErrorMessageBox("Setting error", e.what());
			return false;
		}

		return true;
	}

	/// @brief 指令格納番号
	struct CommandIndex abstract final
	{
		enum : uint8_t
		{
			Unspecified,
			Command,
			Arg1,
			Arg2,
			Arg3,
			Arg4,
			Arg5,
			Arg6,
			WaitType,
			Text,
			PageCtrl,
			Voice,
			WindowType,
			Arg7
		};
	};

	/// @brief 素材階層
	struct ResourcePath
	{
		StaticWString32 characterId; /* 恐らく5桁固定 */
		std::wstring sceneAnimationFolderPath;
		std::wstring voiceFileDirectory;
	};

	/// @brief 台本ファイル経路から各種素材階層導出
	static bool DeriveResourcePathFromScriptFilePath(const std::wstring& scriptFilePath, ResourcePath& resourcePath)
	{
		std::wstring_view scritpFileName = path_utility::ExtractFileNameWithoutExtension(scriptFilePath);
		const size_t nCharacterIdEnd = scritpFileName.find_first_not_of(L"0123456789");
		if (nCharacterIdEnd == std::wstring_view::npos)return false;

		/* 多分今回は'A'だけ。仮に'B'が追加されたら"_0"部分が"_1"になると思われる。 */
		const wchar_t cSceneType = scritpFileName[nCharacterIdEnd];

		const size_t nSceneDirStart = scriptFilePath.rfind(L"scenarioexcel");
		if (nSceneDirStart == std::wstring::npos)return false;

		wchar_t pathBuffer[kMaxPathLength]{};
		int nWritten = swprintf_s(pathBuffer, 
			LR"(%.*sresources\advscene\texture\character\r18_scenes\r18_%.*s_0\)",
			static_cast<int>(nSceneDirStart), &scriptFilePath[0],
			static_cast<int>(nCharacterIdEnd), &scritpFileName[0]
			);
		if (nWritten == -1)return false;

		resourcePath.characterId.assign(&scritpFileName[0], nCharacterIdEnd);
		resourcePath.sceneAnimationFolderPath.assign(pathBuffer, nWritten);
		resourcePath.voiceFileDirectory.assign(&scriptFilePath[0], nSceneDirStart).append(LR"(resources\advscene\sound\voice\)");

		return true;
	}

} /* namespace rlyeh*/


bool rlyeh::InitialisePlayerSetting()
{
	PlayerSetting playerSetting;
	bool bRet = ReadSettingFile(playerSetting);
	if (bRet)
	{
		g_playerSetting = playerSetting;
	}

	return g_playerSetting.atlasExtension != g_playerSetting.skelExtension;
}

const std::wstring_view& rlyeh::GetScriptFileFilter()
{
	return s_scriptFileFilter;
}

void rlyeh::BuildScriptFilePathList(const std::wstring& selectedScriptFilePath, std::vector<std::wstring>& scriptFilePaths, size_t& nIndex)
{
	std::wstring_view parentFolderPath = path_utility::ExtractParentPath(path_utility::ExtractParentPath(selectedScriptFilePath));
	std::vector<std::wstring> scriptFolderPaths;
	win_filesystem::CreateFilePathList(parentFolderPath, {}, scriptFolderPaths);

	for (const auto& scriptFolderPath : scriptFolderPaths)
	{
		win_filesystem::CreateFilePathList(scriptFolderPath, s_scriptFileFilter, scriptFilePaths);
	}

	const auto iter = std::find(scriptFilePaths.begin(), scriptFilePaths.end(), selectedScriptFilePath);
	if (iter != scriptFilePaths.cend())
	{
		nIndex = std::distance(scriptFilePaths.begin(), iter);
	}
}

void rlyeh::FindSceneAnimationFiles(const std::wstring& animationFolderPath, std::wstring& atlasFilePath, std::wstring& skelFilePath)
{
	std::vector<std::wstring> filePaths;
	win_filesystem::CreateFilePathList(animationFolderPath, L"*", filePaths);
	
	std::wstring_view atlasExtension = g_playerSetting.atlasExtension.string_view();
	std::wstring_view skelExtension = g_playerSetting.skelExtension.string_view();
	for (const auto& filePath : filePaths)
	{
		if (filePath.ends_with(atlasExtension) && atlasFilePath.empty())
		{
			atlasFilePath.assign(filePath);
		}
		else if (filePath.ends_with(skelExtension) && skelFilePath.empty())
		{
			skelFilePath.assign(filePath);
		}

		if (!atlasFilePath.empty() && !skelFilePath.empty())break;
	}
}

bool rlyeh::ReadScenarioScript(const std::wstring& scriptFilePath, std::vector<adv::TextDatum>& textData, std::vector<std::string>& animationNames, std::wstring& animationFolderPath, std::vector<adv::SceneDatum>& sceneData, std::vector<adv::LabelDatum>& labelData)
{
	ResourcePath resourcePath;
	bool bRet = DeriveResourcePathFromScriptFilePath(scriptFilePath, resourcePath);
	if (!bRet)return false;

	std::string scriptFile = win_filesystem::LoadFileAsString(scriptFilePath.data());
	if (scriptFile.empty())return false;

	size_t nLastAnimationIndex = 0;
	StaticWString32 captionBuffer;

	try
	{
		nlohmann::json nlJson = nlohmann::json::parse(scriptFile);
		const nlohmann::json& rows = nlJson.at("importGridList").at(0).at("rows");

		/* 項目名は飛ばす */
		for (size_t i = 1; i < rows.size(); ++i)
		{
			const auto& commandDatum = rows[i].at("strings");
			std::string_view command = commandDatum[CommandIndex::Command];
			if (!command.empty())continue; /* 文章指定・Spine動作指定以外の指令文。 */

			if (commandDatum.size() > CommandIndex::Arg3)
			{
				std::string_view arg3 = commandDatum[CommandIndex::Arg3];
				if (arg3 == "layer_R18") /* Spine動作指定 */
				{
					std::string_view animationName = commandDatum[CommandIndex::Arg2];
					animationNames.emplace_back(animationName);

					/* 対応文章がないので前文章データを上書き */
					if (!captionBuffer.empty() && !sceneData.empty())
					{
						labelData.emplace_back(adv::LabelDatum(std::wstring(captionBuffer.string_view()), sceneData.size() - 1));
						sceneData.back().nAnimationIndex = nLastAnimationIndex;
						captionBuffer.clear();
					}
					nLastAnimationIndex = animationNames.size() - 1;

					const int nWritten = win_text::WidenUtf8InBuffer(animationName.data(), static_cast<int>(animationName.size()), &captionBuffer[0], static_cast<int>(captionBuffer.capacity()));
					if (nWritten != 0)
					{
						captionBuffer.resize(static_cast<size_t>(nWritten));
					}

					continue;
				}
			}
			if (commandDatum.size() > CommandIndex::Text)
			{
				/* 導入部分の文章は格納しない。*/
				if (animationNames.empty())continue;

				adv::TextDatum t;

				std::string_view sceneText = commandDatum[CommandIndex::Text];
				if (sceneText.empty())continue;
				t.sceneText.assign(win_text::WidenUtf8(sceneText.data(), static_cast<int>(sceneText.size())));
				text_utility::EliminateTagInPlace(t.sceneText);

				std::string_view characterName = commandDatum[CommandIndex::Arg1];
				if (!characterName.empty())
				{
					t.characterName.assign(win_text::WidenUtf8(characterName.data(), static_cast<int>(characterName.size())));
				}

				if (commandDatum.size() > CommandIndex::Voice)
				{
					std::string_view voiceFilePath = commandDatum[CommandIndex::Voice];
					if (!voiceFilePath.empty())
					{
						/* Character/ch_XXXXX/R18/vo_R18_XXXXX_YYY で高々39文字 */
						wchar_t pathBuffer[128]{};
						static constexpr size_t kPathBufferSize = std::size(pathBuffer);
						int nWritten = win_text::WidenUtf8InBuffer(voiceFilePath.data(), voiceFilePath.size(), pathBuffer, kPathBufferSize);
						if (nWritten != 0)
						{
							std::replace(pathBuffer, pathBuffer + nWritten, L'/', L'\\');
							t.voiceFilePath
								.assign(resourcePath.voiceFileDirectory)
								.append(pathBuffer, nWritten)
								.append(g_playerSetting.voiceExtension.string_view());
						}
					}
				}

				textData.push_back(std::move(t));
				sceneData.emplace_back(adv::SceneDatum(textData.size() - 1, nLastAnimationIndex));
				if (!captionBuffer.empty())
				{
					labelData.emplace_back(adv::LabelDatum(std::wstring(captionBuffer.string_view()), sceneData.size() - 1));
					captionBuffer.clear();
				}
			}
		}
	}
	catch (const nlohmann::json::exception& e)
	{
		win_dialogue::ShowErrorMessageBox("Parse error", e.what());
	}

	animationFolderPath.assign(resourcePath.sceneAnimationFolderPath);

	return true;
}