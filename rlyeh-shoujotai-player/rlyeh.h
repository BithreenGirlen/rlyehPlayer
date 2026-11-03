#ifndef RLYER_H_
#define RLYER_H_

#include <string>
#include <vector>

#include "adv.h"

namespace rlyeh
{
	/// @brief 設定ファイル読み込み
	bool InitialisePlayerSetting();

	/// @brief 台本ファイル名書式取得
	const std::wstring_view& GetScriptFileFilter();

	/// @brief 台本ファイル一覧経路作成
	void BuildScriptFilePathList(const std::wstring& selectedScriptFilePath, std::vector<std::wstring>& scriptFilePaths, size_t& nIndex);
	
	/// @brief Spineファイル経路取得。重ね書きはないと思うので単一で
	void FindSceneAnimationFiles(const std::wstring& animationFolderPath, std::wstring& atlasFilePath, std::wstring& skelFilePath);

	/// @brief 台本読み込み
	bool ReadScenarioScript(
		const std::wstring& scriptFilePath,
		std::vector<adv::TextDatum>& textData,
		std::vector<std::string>& animationNames,
		std::wstring& animationFolderPath,
		std::vector<adv::SceneDatum>& sceneData,
		std::vector<adv::LabelDatum>& labelData
	);
}

#endif // !RLYER_H_
