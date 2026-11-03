#ifndef ADV_H_
#define ADV_H_

#include <string>

namespace adv
{
	struct TextDatum
	{
		std::wstring characterName;
		std::wstring sceneText;
		std::wstring voiceFilePath;
	};

	struct SceneDatum
	{
		size_t nTextIndex = 0;
		size_t nAnimationIndex = 0;
	};

	struct LabelDatum
	{
		std::wstring caption;
		size_t nSceneIndex = 0;
	};
}

#endif // !ADV_H_
