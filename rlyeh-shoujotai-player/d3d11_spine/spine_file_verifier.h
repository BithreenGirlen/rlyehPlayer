#ifndef SPINE_FILE_VERIFIER_H_
#define SPINE_FILE_VERIFIER_H_

#if (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L) || (defined(__cplusplus) && __cplusplus >= 202002L)
	#define ALLOW_CPP20_STL
	#include <string_view>
#endif

namespace spine_file_verifier
{
	enum class SkeletonFormat
	{
		Neither, /* The file is not Spine skeleton file. */
		Json,
		Binary,
	};

	/* Skeleton format and its version */
	struct SkeletonMetadata
	{
		SkeletonFormat skeletonFormat = SkeletonFormat::Neither;
#if defined (ALLOW_CPP20_STL)
		std::string_view version;
#else
		const unsigned char* version = nullptr;
		size_t versionLength = 0;
#endif
	};

	SkeletonMetadata VerifySkeletonFileData(const unsigned char* pFileData, size_t dataLength);

	/// @brief Atlas記載全ファイル名
	struct AtlasMetaData
	{
		/// @brief 最大画像数
		/// @remark 大きさ16384 x 16384を2048x2048単位8分割出力に加えマスク画像を加えたものを最大数と想定
		static constexpr size_t kMaxTextureCount = 9;
#if defined (ALLOW_CPP20_STL)
		std::string_view textureNames[kMaxTextureCount];
#else
		struct RefString
		{
			const char* p = nullptr;
			size_t length = 0;
		};
		RefString textureNames[kMaxTextureCount];
#endif
		size_t textureNameCount = 0;
	};

	/// @brief Atlas指定ファイル名の抜粋。メモリ読み込み用途
	AtlasMetaData FindAtlasTextureNames(const char* pFileData, size_t dataLepgth);
}

#endif // !SPINE_FILE_VERIFIER_H_
