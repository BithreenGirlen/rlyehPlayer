
#include <Windows.h>

#include "d3d11_spine_extension.h"

#include <spine/SpineString.h> /* spine::String::buffer() */

void* CD3d11SpineExtension::_alloc(size_t size, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	if (size == 0) return nullptr;

	return ::malloc(size);
}

void* CD3d11SpineExtension::_calloc(size_t size, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	if (size == 0) return nullptr;

	void* p = ::malloc(size);
	if (p != nullptr)
	{
		::memset(p, 0, size);
	}

	return p;
}

void* CD3d11SpineExtension::_realloc(void* ptr, size_t size, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	if (size == 0) return nullptr;

	void* p = nullptr;
	if (ptr == nullptr)
	{
		p = ::malloc(size);
	}
	else
	{
		p = ::realloc(ptr, size);
	}

	return p;
}

void CD3d11SpineExtension::_free(void* mem, [[maybe_unused]] const char* file, [[maybe_unused]] int line)
{
	::free(mem);
}

char* CD3d11SpineExtension::_readFile(const spine::String& path, int* length)
{
	HANDLE hFile = INVALID_HANDLE_VALUE;
	DWORD nFileSize = INVALID_FILE_SIZE;
	DWORD nReadBytes = 0;
	LPVOID pFile = nullptr;
	BOOL iRet = FALSE;
	int nWritten = 0;

	WCHAR filePathBuffer[1024]{};
	static constexpr size_t kFilePathBufferSize = sizeof(filePathBuffer) / sizeof(WCHAR);

	nWritten = ::MultiByteToWideChar(CP_UTF8, 0, path.buffer(), path.length(), filePathBuffer, kFilePathBufferSize);
	if (nWritten == 0)return nullptr;

	hFile = ::CreateFileW(filePathBuffer, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFile == INVALID_HANDLE_VALUE)goto end;

	nFileSize = ::GetFileSize(hFile, nullptr);
	if (nFileSize == INVALID_FILE_SIZE)goto end;

	pFile = ::malloc(nFileSize);
	if (pFile == nullptr)goto end;

	iRet = ::ReadFile(hFile, pFile, nFileSize, &nReadBytes, nullptr);
	if (iRet == FALSE)goto end; /* To suppress warning C28193 */

end:
	if (hFile != INVALID_HANDLE_VALUE)
	{
		::CloseHandle(hFile);
	}

	if (length != nullptr)
	{
		*length = static_cast<int>(nFileSize);
	}

	return static_cast<char*>(pFile);
}

spine::SpineExtension* spine::getDefaultExtension()
{
	static CD3d11SpineExtension s_d3d11SpineExtension;

	return &s_d3d11SpineExtension;
}
