#ifndef D3D11_SPINE_EXTENSION_H_
#define D3D11_SPINE_EXTENSION_H_

#include <spine/Extension.h>

/// @brief Spine APIから呼び出されるメモリ割り当て・ファイル読み取り実装
class CD3d11SpineExtension : public spine::SpineExtension
{
protected:
	void* _alloc(size_t size, const char* file, int line) override;
	void* _calloc(size_t size, const char* file, int line) override;
	void* _realloc(void* ptr, size_t size, const char* file, int line) override;
	void _free(void* mem, const char* file, int line) override;
	char* _readFile(const spine::String& path, int* length) override;
};

#endif // !D3D11_SPINE_EXTENSION_H_
