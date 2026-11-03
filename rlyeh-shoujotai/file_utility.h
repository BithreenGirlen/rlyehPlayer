#ifndef FILE_UTILITY_H_
#define FILE_UTILITY_H_

#include <string>

/// @brief ファイル経路操作にて動的確保を行わないファイル読み書き関数群
namespace file_utility
{
	/// @brief 実行プロセスの階層取得
	/// @return 大域変数を指す階層
	std::string_view GetBasePath();

	/// @brief 静的バッファを用いてフォルダ作成
	/// @param directoryName 作成する階層名
	/// @param dst 書き込み先
	/// @param nWritten 書き込まれた文字列長
	/// @param dstSize 書き込み先の容量
	/// @param basePath 基底階層。空文字列の場合実行プロセスの階層。
	/// @return 成功時true, 失敗時false
	bool CreateDirectoryStatic(std::string_view directoryName, char* dst, size_t dstSize, size_t& nWritten, std::string_view basePath = {});

	std::string LoadFileAsString(const char* filePath);
	bool SaveFileData(const void* pData, unsigned long dataLength, std::string_view filePath);
	bool DoesFilePathExist(std::string_view filePath);

	std::string LoadInternetResourceAsString(const char* url);
	bool SaveInternetResourceToFile(std::string_view url, std::string_view folderPath = {}, std::string_view fileName = {}, bool toOverwrite = false);
	bool SaveInternetResourceToFileCreatingNestedFolder(std::string_view url, std::string_view fileName = {}, std::string_view basePath = {}, int depth = 0);
	uint64_t GetInternetResoureSize(const char* url);
}

#endif // !FILE_UTILITY_H_
