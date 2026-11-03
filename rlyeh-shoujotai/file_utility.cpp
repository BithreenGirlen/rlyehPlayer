

#include <Windows.h>
#include <urlmon.h>
#include <atlbase.h>

#include "file_utility.h"

#pragma comment(lib, "Urlmon.lib")
#pragma comment(lib, "shlwapi.lib")

/* 内部用 */
namespace file_utility
{
	/* 最大経路長 */
	static constexpr size_t kMaxPathLength = 512;

	/* 伝文出力 */
	static void WriteMessage(const char* format, ...)
	{
		char sBuffer[4096]{};
		constexpr size_t bufferSize = sizeof(sBuffer) - 1;

		SYSTEMTIME tm;
		::GetLocalTime(&tm);

		int timeLen = sprintf_s(sBuffer, "%02d:%02d:%02d:%03d ", tm.wHour, tm.wMinute, tm.wSecond, tm.wMilliseconds);
		if (timeLen == -1)return;

		va_list args;
		va_start(args, format);
		vsprintf_s(sBuffer + timeLen, bufferSize - timeLen, format, args);
		va_end(args);

		strcat_s(sBuffer, "\n");

		printf(sBuffer);
	}
	/// @brief 拡張子を含むファイル名を抜粋
	static std::string_view ExtractFileName(std::string_view path)
	{
		size_t nPos1 = path.find_last_of("\\/");
		if (nPos1 != std::string::npos)
		{
			++nPos1;
			size_t nPos2 = path.find('?', nPos1);

			return nPos2 == std::string::npos ?
				path.substr(nPos1) :
				path.substr(nPos1, nPos2 - nPos1);
		}

		return path;
	}
	/// @brief 親階層を抜粋
	static std::string_view ExtractParentDirectory(std::string_view path)
	{
		size_t nPos = path.find_last_of("\\/");
		if (nPos == std::string_view::npos)nPos = 0;

		return path.substr(0, nPos);
	}
}


std::string_view file_utility::GetBasePath()
{
	static char s_basePath[kMaxPathLength]{};
	static size_t s_basePathLength = 0;
	if (s_basePath[0] == '\0')
	{
		DWORD length = ::GetModuleFileNameA(nullptr, s_basePath, sizeof(s_basePath));
		char* pEnd = s_basePath + length;
		for (; pEnd != s_basePath; --pEnd)
		{
			if (*pEnd == '\\' || *pEnd == '/')break;
		}

		char* pFileName = pEnd + 1;
		size_t fileNameLength = s_basePath + length - pFileName;
		memset(pFileName, '\0', fileNameLength);

		s_basePathLength = pFileName - s_basePath;
	}

	return std::string_view(s_basePath, s_basePathLength);
}

bool file_utility::CreateDirectoryStatic(std::string_view directoryName, char* dst, size_t dstSize, size_t& nWritten, std::string_view basePath)
{
	if (basePath.empty())
	{
		basePath = GetBasePath();
	}

	if (dstSize < basePath.size())return false;

	memcpy(dst, basePath.data(), basePath.size());
	nWritten = basePath.size();

	size_t nRead = 0;
	if (directoryName[0] == '\\' || directoryName[0] == '/')++nRead;

	for (;;)
	{
		size_t nPos = directoryName.find_first_of("\\/", nRead);
		if (nPos == std::string_view::npos)
		{
			const char* pRead = directoryName.data() + nRead;
			size_t nLength = directoryName.size() - nRead;
			if (dstSize < nWritten + nLength + 1)return false;

			memcpy(dst + nWritten, pRead, nLength);
			nWritten += nLength;
			dst[nWritten++] = '\\';
			dst[nWritten] = '\0';

			::CreateDirectoryA(dst, nullptr);

			break;
		}

		const char* pRead = &directoryName[nRead];
		size_t nLength = nPos - nRead;
		if (dstSize < nWritten + nLength + 1)return false;

		memcpy(dst + nWritten, pRead, nLength);
		nWritten += nLength;
		dst[nWritten++] = '\\';

		::CreateDirectoryA(dst, nullptr);

		nRead = nPos + 1;
	}

	return true;
}

std::string file_utility::LoadFileAsString(const char* filePath)
{
	std::string fileData;
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD ulSize = INVALID_FILE_SIZE;
	DWORD ulRead = 0;
	BOOL iRet = FALSE;

	hFile = ::CreateFileA(filePath, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)goto end;

	ulSize = ::GetFileSize(hFile, nullptr);
	if (ulSize == INVALID_FILE_SIZE)goto end;

	fileData.resize(ulSize);
	iRet = ::ReadFile(hFile, &fileData[0], ulSize, &ulRead, nullptr);
	/* To suppress warning C28193 */
	if (iRet == FALSE)goto end;

end:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}

	return fileData;
}

bool file_utility::SaveFileData(const void* pData, unsigned long dataLength, std::string_view filePath)
{
	if (filePath.empty())return false;

	BOOL iRet = 0;
	HANDLE hFile = ::CreateFileA(filePath.data(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::SetFilePointer(hFile, NULL, nullptr, FILE_END);

		DWORD nWritten = 0;
		iRet = ::WriteFile(hFile, pData, static_cast<DWORD>(dataLength), &nWritten, nullptr);
		::CloseHandle(hFile);
	}

	if (iRet > 0)
	{
		WriteMessage("%.*s %s", static_cast<int>(filePath.size()), filePath.data(), "success");
	}
	else
	{
		WriteMessage("%.*s %s", static_cast<int>(filePath.size()), filePath.data(), "failed");
	}

	return iRet > 0;
}

bool file_utility::DoesFilePathExist(std::string_view filePath)
{
	BOOL iRet = ::PathFileExistsA(filePath.data());
	if (iRet)
	{
		WriteMessage("%.*s %s", static_cast<int>(filePath.size()), filePath.data(), "already exists.");
	}

	return iRet == TRUE;
}

std::string file_utility::LoadInternetResourceAsString(const char* url)
{
	if (url == nullptr)return{};

	CComPtr<IStream> pStream;
	HRESULT hr = ::URLOpenBlockingStreamA(nullptr, url, &pStream, 0, nullptr);
	if (hr == S_OK)
	{
		STATSTG stat;
		hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
		if (hr == S_OK)
		{
			std::string str(stat.cbSize.QuadPart, '\0');
			ULONGLONG nTotalRead = 0;
			for (;;)
			{
				DWORD ulRead = 0;
				ULONG ulToBeRead = static_cast<ULONG>(stat.cbSize.QuadPart - nTotalRead);
				hr = pStream->Read(&str[nTotalRead], ulToBeRead, &ulRead);
				if (FAILED(hr))break;

				nTotalRead += ulRead;
				if (nTotalRead >= stat.cbSize.QuadPart)break;
			}

			return str;
		}
	}

	return {};
}

bool file_utility::SaveInternetResourceToFile(std::string_view url, std::string_view folderPath, std::string_view fileName, bool toOverwrite)
{
	/* Todo: escape more characters which cannot be used in filename. */
	std::string_view truncatedFileName =
		fileName.empty() ?
		ExtractFileName(url) : 
		ExtractFileName(fileName);

	if (folderPath.empty())
	{
		folderPath = GetBasePath();
	}

	char filePath[kMaxPathLength]{};
	size_t filePathLength = 0;
	constexpr size_t filePathSize = sizeof(filePath) - 1;
	if (filePathSize < folderPath.size() + truncatedFileName.size() + 1)return false;
	
	memcpy(&filePath[filePathLength], folderPath.data(), folderPath.size());
	filePathLength += folderPath.size();

	memcpy(&filePath[filePathLength], truncatedFileName.data(), truncatedFileName.size());
	filePathLength += truncatedFileName.size();
	filePath[filePathLength++] = '\0';

	if (toOverwrite || !::PathFileExistsA(filePath))
	{
		HRESULT hr = ::URLDownloadToFileA(nullptr, url.data(), filePath, 0, nullptr);
		if (hr == S_OK)
		{
			WriteMessage("%.*s %s", static_cast<int>(url.size()), url.data(), "success");
			return true;
		}
	}
	else
	{
		WriteMessage("%.*s %s", static_cast<int>(truncatedFileName.size()), truncatedFileName.data(), "already exists.");
		return true;
	}

	WriteMessage("%.*s %s", static_cast<int>(url.size()), url.data(), "failed");

	return false;
}

bool file_utility::SaveInternetResourceToFileCreatingNestedFolder(std::string_view url, std::string_view fileName, std::string_view basePath, int depth)
{
	if (basePath.empty())
	{
		basePath = GetBasePath();
	}

	const auto ExtractHostDir = [&url, &depth]()
		-> std::string_view
		{
			constexpr const char key[] = "//";
			size_t nPos = url.find(key);
			if (nPos == std::string_view::npos)nPos = 0;
			nPos += sizeof(key);

			size_t nPos2 = nPos;
			do
			{
				nPos = url.find('/', nPos2);
				if (nPos2 == std::string_view::npos)break;
				else nPos2 = nPos + 1;

				--depth;
			} while (depth >= 0);

			size_t nPos3 = url.find_last_of('/');
			if (nPos3 == std::string_view::npos)nPos3 = url.size();

			return url.substr(nPos2, nPos3 - nPos2);
		};

	char folderPath[kMaxPathLength]{};
	size_t folderPathLength = 0;
	if (fileName.empty())
	{
		std::string_view hostDir = ExtractHostDir();
		CreateDirectoryStatic(hostDir, folderPath, sizeof(folderPath), folderPathLength, basePath);
	}
	else
	{
		std::string_view parentDir = ExtractParentDirectory(fileName);
		CreateDirectoryStatic(parentDir, folderPath, sizeof(folderPath), folderPathLength, basePath);
	}

	return SaveInternetResourceToFile(url, folderPath, fileName);
}

uint64_t file_utility::GetInternetResoureSize(const char* url)
{
	CComPtr<IStream> pStream;
	HRESULT hr = ::URLOpenBlockingStreamA(nullptr, url, &pStream, 0, nullptr);
	if (hr == S_OK)
	{
		STATSTG stat;
		hr = pStream->Stat(&stat, STATFLAG_DEFAULT);
		if (hr == S_OK)
		{
			return static_cast<uint64_t>(stat.cbSize.QuadPart);
		}
	}

	return 0;
}
