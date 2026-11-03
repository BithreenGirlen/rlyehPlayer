

#include "file_utility.h"
#include "json_minimal.h"
#include "win_http_session.h"

struct AssetBundle
{
	std::string_view path;
	std::string_view hash;
};

static constexpr const char g_rawAsseBundleDir[] =
{
	0x68, 0x74, 0x74, 0x70, 0x73, 0x3A, 0x2F, 0x2F, 0x61, 0x73, 0x73, 0x65, 0x74, 0x73, 0x2E, 0x63,
	0x74, 0x68, 0x75, 0x6C, 0x68, 0x75, 0x2D, 0x72, 0x6F, 0x67, 0x2E, 0x6E, 0x65, 0x74, 0x2F, 0x61,
	0x73, 0x73, 0x65, 0x74, 0x62, 0x75, 0x6E, 0x64, 0x6C, 0x65, 0x73, 0x2F, 0x00
};
static constexpr const char g_rawBuildPlatformDir[] =
{
	0x2F, 0x77, 0x65, 0x62, 0x67, 0x6C, 0x5F, 0x72, 0x31, 0x38, 0x2F, 0x00
};

static constexpr std::string_view g_assetBundleDir = std::string_view(g_rawAsseBundleDir, std::size(g_rawAsseBundleDir) - 1);
static constexpr std::string_view g_buildPlatformDir(g_rawBuildPlatformDir, std::size(g_rawBuildPlatformDir) - 1);

static std::string RequestAssetBundleListVersion()
{
	constexpr wchar_t url[] =
	{
		0x68, 0x74, 0x74, 0x70, 0x73, 0x3A, 0x2F, 0x2F, 0x61, 0x70, 0x69, 0x2E, 0x63, 0x74, 0x68, 0x75,
		0x6C, 0x68, 0x75, 0x2D, 0x72, 0x6F, 0x67, 0x2E, 0x6E, 0x65, 0x74, 0x2F, 0x61, 0x70, 0x69, 0x2F,
		0x61, 0x73, 0x73, 0x65, 0x74, 0x5F, 0x62, 0x75, 0x6E, 0x64, 0x6C, 0x65, 0x2F, 0x76, 0x65, 0x72,
		0x73, 0x69, 0x6F, 0x6E, 0x00
	};
	constexpr wchar_t requestHeader[] = L"Content-Type: application/json\r\n";
	static constexpr const char s_rawPlayload[] =
	{
		0x7B, 0x22, 0x63, 0x76, 0x72, 0x22, 0x3A, 0x20, 0x22, 0x31, 0x2E, 0x30, 0x2E, 0x33, 0x22, 0x2C,
		0x20, 0x22, 0x70, 0x72, 0x6F, 0x76, 0x69, 0x64, 0x65, 0x72, 0x22, 0x3A, 0x20, 0x22, 0x64, 0x6D,
		0x6D, 0x22, 0x7D, 0x00
	};
	constexpr std::string_view s_payload(s_rawPlayload, std::size(s_rawPlayload) - 1);

	ResponseData r;

	bool bRet = win_http_session::RequestHttpPost(url, requestHeader, s_payload.data(), static_cast<unsigned long>(s_payload.length()), r);
	if (bRet)
	{
		if (r.header.find(L"HTTP/1.1 200 OK") != std::wstring::npos)
		{
			return r.content;
		}
	}

	return {};
}

static std::string DownloadAssetBundleList()
{
	std::string versionRespose = RequestAssetBundleListVersion();
	const char* p = &versionRespose[0];
	const char* pStart = nullptr, * pEnd = nullptr;
	bool bRet = json_minimal::FindValueByName(p, "version", &pStart, &pEnd);
	if (!bRet)return {};

	char versionBuffer[64]{};
	size_t versionLength = pEnd - pStart;
	if (versionLength >= sizeof(versionBuffer))return {};
	memcpy(versionBuffer, pStart, versionLength);
	versionBuffer[versionLength] = '\0';

	const auto Unescape = [&versionBuffer]()
		-> size_t
		{
			const char* pSrc = versionBuffer;
			char* pDst = versionBuffer;
			while (*pSrc)
			{
				if (*pSrc == '\\')
				{
					pSrc++;
					if (!*pSrc) break;

					switch (*pSrc)
					{
					case 'b':  *pDst++ = '\b'; break;
					case 'f':  *pDst++ = '\f'; break;
					case 'n':  *pDst++ = '\n'; break;
					case 'r':  *pDst++ = '\r'; break;
					case 't':  *pDst++ = '\t'; break;
					default: *pDst++ = *pSrc; break;
					}
				}
				else
				{
					*pDst++ = *pSrc;
				}
				pSrc++;
			}

			*pDst = '\0';
			return pDst - versionBuffer;
		};
	Unescape();

	char sBuffer[512]{};
	sprintf_s(sBuffer, "%.*sproduction/%s%.*sablist.json",
		static_cast<int>(g_assetBundleDir.length()), g_assetBundleDir.data(),
		versionBuffer,
		static_cast<int>(g_buildPlatformDir.length()), g_buildPlatformDir.data()
	);

	return file_utility::LoadInternetResourceAsString(sBuffer);
}

static void ReadAssetBundleList(const std::string& file, std::vector<AssetBundle>& assetBundles, std::string_view& version)
{
	if (file.empty())return;

	const char* p = &file[0];
	const char* pStart = nullptr, * pEnd = nullptr;

	bool bRet = json_minimal::FindValueByName(p, "baseVersion", &pStart, &pEnd);
	if (!bRet)return;

	version = std::string_view(pStart, pEnd - pStart);

	bRet = json_minimal::FindNextArray(&p, "data", &pStart, &pEnd);
	if (!bRet)return;

	p = pStart;
	for (;;)
	{
		AssetBundle s;

		bRet = json_minimal::FindNextObject(&p, nullptr, &pStart, &pEnd);
		if (!bRet)break;

		const char* p1 = pStart;
		bRet = json_minimal::FindValueByName(p1, "path", &pStart, &pEnd);
		s.path = std::string_view(pStart, pEnd - pStart);

		bRet = json_minimal::FindValueByName(p1, "hash", &pStart, &pEnd);
		s.hash = std::string_view(pStart, pEnd - pStart);

		assetBundles.push_back(std::move(s));
	}
}

static void DownloadAssetBundles()
{
	std::string file = DownloadAssetBundleList();
	if (file.empty())return;

#ifdef _DEBUG
	file_utility::SaveFileData(file.data(), static_cast<unsigned long>(file.size()), "ablist.json");
#endif

	std::vector<AssetBundle> assetBundles;
	std::string_view version;
	ReadAssetBundleList(file, assetBundles, version);

	char folderPathBuffer[512]{};
	size_t nWritten = 0;
	constexpr size_t bufferSize = sizeof(folderPathBuffer) - 1;
	bool bRet = file_utility::CreateDirectoryStatic("AssetBundles", folderPathBuffer, bufferSize, nWritten);

	char urlBuffer[512]{};
	for (const auto& assetBundle : assetBundles)
	{
		sprintf_s(urlBuffer, "%.*sproduction/ver_%.*s%.*s%.*s%.*s",
			static_cast<int>(g_assetBundleDir.length()), g_assetBundleDir.data(),
			static_cast<int>(version.length()), version.data(),
			static_cast<int>(g_buildPlatformDir.length()), g_buildPlatformDir.data(),
			static_cast<int>(assetBundle.hash.size()), assetBundle.hash.data(),
			static_cast<int>(assetBundle.path.size()), assetBundle.path.data()
		);
		file_utility::SaveInternetResourceToFile(urlBuffer, folderPathBuffer);
	}
}

int main(int argc, char* argv[])
{
	DownloadAssetBundles();
}