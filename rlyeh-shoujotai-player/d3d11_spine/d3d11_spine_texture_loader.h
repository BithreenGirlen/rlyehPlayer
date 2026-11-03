#ifndef D3D11_SPINE_TEXTURE_LOADER_H
#define D3D11_SPINE_TEXTURE_LOADER_H

#include <Windows.h>
#include <d3d11.h>

#include <spine/TextureLoader.h>

class CD3D11SpineTextureLoader : public spine::TextureLoader
{
public:
	/// @brief Enable if Direct2D and DirectWrite be used.
	static void SetFormatAsBgraOnLoading(bool toLoadAsBgra32) noexcept;
	static bool IsTextureToBeLoadedAsBgra32() noexcept;

	/// @brief Enable if alpha is to be multiplied on loading.
	///        For Spine 4.0 and later, applied only when spine::AtlasPage::pma is false.
	///        For Spine 3.8 and older, always applied.
	static void EnableConversionToPma(bool toLoadAsPma) noexcept;
	static bool IsConversionToPmaEnabled() noexcept;
;
	CD3D11SpineTextureLoader() = default;
	virtual ~CD3D11SpineTextureLoader() = default;

	/// @brief *pOutImage should be casted to the pointer of the type of interest. 
	/// @param pUserDatum User data passed when to register the callback.
	/// @param textureFilePath Texture filepath in UTF-8.
	/// @param filePathLength The length of texture filepath.
	/// @param pOutImage An out pointer which should be casted to IWICBitmap**.
	typedef void(*LoadCallback)(void* pUserDatum, const char* textureFilePath, size_t filePathLength, void* pOutImage);
	
	void load(spine::AtlasPage& atlasPage, const spine::String& textureFilePath) override;
	void unload(void* texture) override;

	void setD3D11Device(ID3D11Device* pD3D11Device) noexcept;

	/// @brief Set a load callback
	void setTextureLoadCallback(LoadCallback pFunc, void* pUserDatum) noexcept;
private:
	ID3D11Device* m_pStotedD3D11Device = nullptr;

	LoadCallback m_pTextureLoadCallback = nullptr;
	void* m_pCallbackUserDatum = nullptr;
};

#endif // !D3D11_SPINE_TEXTURE_LOADER_H
