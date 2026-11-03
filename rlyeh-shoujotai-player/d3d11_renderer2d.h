#ifndef D3D11_RENDERER2D_H_
#define D3D11_RENDERER2D_H_

#include <Windows.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include <dcommon.h> /* To minimise user-defined structs. */


namespace d3d11_renderer2d
{
	/// @brief 混色因数
	enum class BlendFactor : unsigned char
	{
		Zero = 1,
		One = 2,
		SrcColor = 3,
		InvSrcColor = 4,
		SrcAlpha = 5,
		InvSrcAlpha = 6,
		DstAlpha = 7,
		InvDstAlpha = 8,
		DstColor = 9,
		InvDstColor = 10,
		SrcAlphaSat = 11,
		BlendFactor = 14,
		InvBlendFactor = 15,
		Src1Color = 16,
		InvSrc1Color = 17,
		Src1Alpha = 18,
		InvSrc1Alpha = 19,
	};

	/// @brief 混色演算子
	enum class BlendOperation : unsigned char
	{
		Add = 1,
		Substract = 2,
		RevSubstract = 3,
		Min = 4,
		Max = 5
	};

	/// @brief 混色法
	enum class BlendMode : unsigned char
	{
		NoBlend = 0, /* dstRGBA = srcRGBA */
		Spine_Normal, /* dstRGB = (srcRGB * srcA) + (dstRGB * dstA); dstA = srcA + dstA * (1 - secA) */
		Spine_NormalPma, /* dstRGBA = srcRGBA + dstRGBA * (1 - srcRGBA) */
		Spine_Add, /* dstRGB = (srcRGB * srcA) + dstRGB; dstA =  srcA + dstA */
		Spine_AddPma, /* dstRGBA = srcRGBA + dstRGBA */
		Spine_Multiply, /* dstRGB = (srcRGB * dstRGB) + dstRGB * (1 - srcA); dstA = srcA + dstA * (1 - secA) */
		Spine_Screen, /* dstRGB = srcRGB + dstRGB * (1 - srcRGB); dstA = srcA + dstA * (1 - secA) */
		Custom, /* Custom blend mode using the same factor both for colours and alpha */
		CustomSeparated /* Custom blend mode using separate factors for colours and alpha each */
	};

	/// @brief 補間法
	enum class SamplerState : unsigned char
	{
		Bilenear_Clamp = 0,
		Nearest_Clamp
	};

	/// @brief 二次元図形の頂点データ
	struct Vertex2D
	{
		D2D_VECTOR_2F pos; /* 位置 */
		D2D_VECTOR_4F colour; /* RGBA順の色 */
		D2D_VECTOR_2F tex; /* UV座標 */
	};
	/// @brief 三角形の頂点番号
	struct TriangleIndex
	{
		UINT16 i0;
		UINT16 i1;
		UINT16 i2;
	};

	class CD3D11Renderer2D
	{
	public:
		CD3D11Renderer2D() noexcept = default;
		~CD3D11Renderer2D() noexcept;

		/// @brief 初期化。いずれかの引数をnullptrで渡した場合、割り当て済みデータを全解放。
		bool initialise(ID3D11Device* pD3d11Device, ID3D11DeviceContext* pD3d11DeviceContext);
		/// @brief 初期化に成功しているか否か
		bool hasBeenInitialised() const noexcept;

		void updateViewport(FLOAT fWidth, FLOAT fHeight) noexcept;
		void setupRenderState() noexcept;

		/// @brief 描画を行う際の混色法指定
		void setBlendMode(BlendMode blendMode) const noexcept;

		/// @brief 指定位置への紋像貼り付け
		bool drawTexture(ID3D11ShaderResourceView* pD3D11TextureView, FLOAT fPosX = 0.f, FLOAT fPosY = 0.f) noexcept;

		/// @brief 頂点番号を利用した三角網の描画
		/// @param pVertices 三角形の頂点データ配列
		/// @param verticesCount 三角形の頂点データ数
		/// @param pTriangleIndides 頂点番号配列
		/// @param indicesCount 頂点番号数
		/// @param pD3D11Texture 貼り付ける紋像
		/// @return 成功時true、失敗時false
		bool drawIndexedPolygon(const Vertex2D* pVertices, UINT32 verticesCount, const TriangleIndex* pTriangleIndides, UINT32 indicesCount, ID3D11ShaderResourceView* pD3D11Texture);

		/// @brief 描画色と透過度とで同じ演算式を用いる混色法を新たに作成
		bool createCustomBlendMode(BlendFactor srcRGBA, BlendFactor dstRGBA, BlendOperation opRGBA) noexcept;
		/// @brief 描画色と透過度とで異なる演算式を用いる混色法を新たに作成
		bool createCustomSeparatedBlendMode(BlendFactor srcRGB, BlendFactor dstRGB, BlendOperation opRGB, BlendFactor srcA, BlendFactor dstA, BlendOperation opA) noexcept;
	private:
		static constexpr size_t kBlendStateCount = static_cast<size_t>(BlendMode::CustomSeparated) + 1ULL;
		static constexpr size_t kSamplerStateCount = static_cast<size_t>(SamplerState::Nearest_Clamp) + 1ULL;

		ID3D11Device* m_pStoredD3d11Device = nullptr;
		ID3D11DeviceContext* m_pStoredD3d11DeviceContext = nullptr;

		ID3D11VertexShader* m_pVertexShader2D = nullptr;
		ID3D11InputLayout* m_pVertexInputLayout2D = nullptr;
		ID3D11Buffer* m_pVertexConstantBuffer2D = nullptr;

		ID3D11PixelShader* m_pPixelShader2D = nullptr;

		ID3D11Buffer* m_pVertexBuffer = nullptr;
		UINT m_nVertexBufferCapacity = 0;
		ID3D11Buffer* m_pIndexBuffer = nullptr;
		UINT m_nIndexBufferCapacity = 0;

		ID3D11RasterizerState* m_pRasterizerState = nullptr;
		ID3D11DepthStencilState* m_pDepthStencilState = nullptr;

		ID3D11BlendState* m_pBlendStates[kBlendStateCount]{};
		ID3D11SamplerState* m_pSamplterStates[kSamplerStateCount]{};

		/// @brief 全解放
		void releaseAll() noexcept;

		bool createPredefinedBlendModes() noexcept;
		bool createPredefinedSamplerStates() noexcept;

		void releaseVertexBuffer() noexcept;
		bool checkVertexBufferCapacity(UINT32 vertexCount) noexcept;
		bool recreateVertexBuffer(UINT32 verticesCount) noexcept;

		void releaseIndexBuffer() noexcept;
		bool checkIndexBufferCapacity(UINT32 indiceCount) noexcept;
		bool recreateIndexBuffer(UINT32 indicesCount) noexcept;
	};
}

#endif // !D3D11_RENDERER2D_H_
