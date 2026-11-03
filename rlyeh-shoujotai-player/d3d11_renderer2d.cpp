

#include <d3dcompiler.h>
#include <atlbase.h>

#include "d3d11_renderer2d.h"
#include "d3d11_spine/d3d11_spine_blend_mode.h"

#if defined(_MSVC_LANG)
	#pragma comment (lib, "d3dcompiler.lib")
#endif

/* 内部用*/
namespace d3d11_renderer2d
{
	struct VertexConstant2D
	{
		float transform[4][4];
	};
}

d3d11_renderer2d::CD3D11Renderer2D::~CD3D11Renderer2D() noexcept
{
	releaseAll();
}

bool d3d11_renderer2d::CD3D11Renderer2D::initialise(ID3D11Device* pD3d11Device, ID3D11DeviceContext* pD3d11DeviceContext)
{
	releaseAll();
	if (pD3d11Device == nullptr || pD3d11DeviceContext == nullptr)return false;

	m_pStoredD3d11Device = pD3d11Device;
	m_pStoredD3d11DeviceContext = pD3d11DeviceContext;

	static constexpr const char s_vertexShader2D[] =
		R"(
			cbuffer vertexBuffer : register(b0)
			{
			  float4x4 ProjectionMatrix;
			};

			struct VS_INPUT
			{
			  float2 pos : POSITION;
			  float4 col : COLOR0;
			  float2 uv  : TEXCOORD0;
			};
			
			struct PS_INPUT
			{
			  float4 pos : SV_POSITION;
			  float4 col : COLOR0;
			  float2 uv  : TEXCOORD0;
			};
			
			PS_INPUT main(VS_INPUT input)
			{
			  PS_INPUT output;
			  output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
			  output.col = input.col;
			  output.uv  = input.uv;
			  return output;
			}
		)";
	static constexpr size_t kVertexShader2DLength = sizeof(s_vertexShader2D) - 1;
	CComPtr<ID3DBlob> pVertexShaderBlob;
	HRESULT hr = ::D3DCompile(s_vertexShader2D, kVertexShader2DLength, nullptr, nullptr, nullptr, "main", "vs_4_0", D3DCOMPILE_OPTIMIZATION_LEVEL1, 0, &pVertexShaderBlob, nullptr);
	if (FAILED(hr))return false;

	hr = m_pStoredD3d11Device->CreateVertexShader(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), nullptr, &m_pVertexShader2D);
	if (FAILED(hr))return false;

	const D3D11_INPUT_ELEMENT_DESC vertexInputLayout2D[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,			0, offsetof(Vertex2D, pos),		D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT,	0, offsetof(Vertex2D, colour),	D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,			0, offsetof(Vertex2D, tex),		D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	/* Avoid using std::size here */
	static constexpr size_t kVertexInputLayoutCount = sizeof(vertexInputLayout2D) / sizeof(vertexInputLayout2D[0]);
	hr = m_pStoredD3d11Device->CreateInputLayout(vertexInputLayout2D, kVertexInputLayoutCount, pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize(), &m_pVertexInputLayout2D);
	if (FAILED(hr))return false;

	const D3D11_BUFFER_DESC vertextConstantBufferDesc
	{
		sizeof(VertexConstant2D),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_CONSTANT_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
		0,
		0
	};
	hr = m_pStoredD3d11Device->CreateBuffer(&vertextConstantBufferDesc, nullptr, &m_pVertexConstantBuffer2D);
	if (FAILED(hr))return false;

	static constexpr const char s_pixelShader2D[] =
		R"(
			struct PS_INPUT
			{
				float4 pos : SV_POSITION;
				float4 col : COLOR0;
				float2 uv  : TEXCOORD0;
			};
			sampler sampler0;
			Texture2D texture0;

			float4 main(PS_INPUT input) : SV_Target
			{
				float4 out_col = input.col * texture0.Sample(sampler0, input.uv);
				return out_col;
			}
		)";
	static constexpr size_t kPixelShader2DLength = sizeof(s_pixelShader2D) - 1;

	CComPtr<ID3DBlob> pPixelShaderBlob;
	hr = ::D3DCompile(s_pixelShader2D, kPixelShader2DLength, nullptr, nullptr, nullptr, "main", "ps_4_0", 0, 0, &pPixelShaderBlob, nullptr);
	if (FAILED(hr))return false;

	hr = m_pStoredD3d11Device->CreatePixelShader(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize(), nullptr, &m_pPixelShader2D);
	if (FAILED(hr))return false;

	if(!createPredefinedSamplerStates())return false;

	const D3D11_RASTERIZER_DESC rasterizerDesc
	{
		D3D11_FILL_SOLID,
		D3D11_CULL_NONE,
		FALSE,
		0,
		0.f,
		0.f,
		TRUE,
		TRUE,
		FALSE,
		FALSE
	};
	hr = m_pStoredD3d11Device->CreateRasterizerState(&rasterizerDesc, &m_pRasterizerState);
	if (FAILED(hr))return false;

	static constexpr D3D11_DEPTH_STENCILOP_DESC s_depthStencilOpDesc
	{
		D3D11_STENCIL_OP_KEEP,
		D3D11_STENCIL_OP_KEEP,
		D3D11_STENCIL_OP_KEEP,
		D3D11_COMPARISON_ALWAYS
	};
	const D3D11_DEPTH_STENCIL_DESC depthStencilDesc
	{
		FALSE,
		D3D11_DEPTH_WRITE_MASK_ALL,
		D3D11_COMPARISON_ALWAYS,
		FALSE,
		0xff,
		0xff,
		s_depthStencilOpDesc,
		s_depthStencilOpDesc
	};
	hr = m_pStoredD3d11Device->CreateDepthStencilState(&depthStencilDesc, &m_pDepthStencilState);
	if (FAILED(hr))return false;

	return createPredefinedBlendModes();
}

bool d3d11_renderer2d::CD3D11Renderer2D::hasBeenInitialised() const noexcept
{
	/* 初期化時に最後に作成するデータなので代表として */
	return m_pBlendStates[static_cast<size_t>(BlendMode::Spine_Screen)] != nullptr;
}

void d3d11_renderer2d::CD3D11Renderer2D::updateViewport(FLOAT fWidth, FLOAT fHeight) noexcept
{
	if (m_pStoredD3d11DeviceContext == nullptr)return;

	const D3D11_VIEWPORT viewPort
	{
		0.f,
		0.f,
		fWidth,
		fHeight,
		0.f,
		1.f
	};
	m_pStoredD3d11DeviceContext->RSSetViewports(1, &viewPort);
	const D3D11_RECT scissorRect
	{
		0,
		0,
		static_cast<LONG>(fWidth),
		static_cast<LONG>(fHeight)
	};
	m_pStoredD3d11DeviceContext->RSSetScissorRects(1, &scissorRect);

	D3D11_MAPPED_SUBRESOURCE mappedConstantBuffer;
	HRESULT hr = m_pStoredD3d11DeviceContext->Map(m_pVertexConstantBuffer2D, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &mappedConstantBuffer);
	if (FAILED(hr))return;

	const float transform[4][4]
	{
		2.f / fWidth, 0.f, 0.f, 0.f,
		0.f, -2.f / fHeight, 0.f, 0.f,
		0.f, 0.f, 0.5f, 0.f,
		-1.f, 1.f, 0.5f, 1.f
	};
	::memcpy(mappedConstantBuffer.pData, transform, sizeof(transform));
	m_pStoredD3d11DeviceContext->Unmap(m_pVertexConstantBuffer2D, 0);
}

void d3d11_renderer2d::CD3D11Renderer2D::setupRenderState() noexcept
{
	if (!hasBeenInitialised())return;

	m_pStoredD3d11DeviceContext->RSSetState(m_pRasterizerState);
	m_pStoredD3d11DeviceContext->OMSetDepthStencilState(m_pDepthStencilState, 0);

	m_pStoredD3d11DeviceContext->IASetInputLayout(m_pVertexInputLayout2D);
	m_pStoredD3d11DeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_pStoredD3d11DeviceContext->VSSetShader(m_pVertexShader2D, nullptr, 0);
	m_pStoredD3d11DeviceContext->VSSetConstantBuffers(0, 1, &m_pVertexConstantBuffer2D);

	m_pStoredD3d11DeviceContext->PSSetShader(m_pPixelShader2D, nullptr, 0);
	m_pStoredD3d11DeviceContext->PSSetSamplers(0, 1, &m_pSamplterStates[static_cast<size_t>(SamplerState::Bilenear_Clamp)]);
}

void d3d11_renderer2d::CD3D11Renderer2D::setBlendMode(BlendMode blendMode) const noexcept
{
	if (m_pStoredD3d11DeviceContext == nullptr)return;

	ID3D11BlendState* pBlendState = m_pBlendStates[static_cast<size_t>(blendMode)];
	if (pBlendState == nullptr)return;

	static constexpr FLOAT s_blendFactor[4] = { 0.f, 0.f, 0.f, 0.f };
	m_pStoredD3d11DeviceContext->OMSetBlendState(pBlendState, s_blendFactor, 0xffffffff);
}

bool d3d11_renderer2d::CD3D11Renderer2D::drawTexture(ID3D11ShaderResourceView* pD3D11TextureView, FLOAT fPosX, FLOAT fPosY) noexcept
{
	if (pD3D11TextureView == nullptr)return false;

	CComPtr<ID3D11Resource> pD3D11Resource;
	pD3D11TextureView->GetResource(&pD3D11Resource);
	if (pD3D11Resource == nullptr)return false;

	CComPtr<ID3D11Texture2D> pD3D11Texture2D;
	HRESULT hr = pD3D11Resource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pD3D11Texture2D);
	if (FAILED(hr))return false;

	D3D11_TEXTURE2D_DESC d3d11Texture2dDesc;
	pD3D11Texture2D->GetDesc(&d3d11Texture2dDesc);

	const FLOAT fWidth = static_cast<FLOAT>(d3d11Texture2dDesc.Width);
	const FLOAT fHeight = static_cast<FLOAT>(d3d11Texture2dDesc.Height);

	const Vertex2D vertices[]
	{
		{
			{ fPosX, fPosY },
			{ 1.f, 1.f, 1.f, 1.f },
			{ 0.f, 0.f }
		},
		{
			{ fPosX + fWidth, fPosY },
			{ 1.f, 1.f, 1.f, 1.f },
			{ 1.f, 0.f }
		},
		{
			{ fPosX, fPosY + fHeight },
			{ 1.f, 1.f, 1.f, 1.f },
			{ 0.f, 1.f }
		},
		{
			{ fPosX + fWidth, fPosY + fHeight },
			{ 1.f, 1.f, 1.f, 1.f },
			{ 1.f, 1.f }
		}
	};
	static constexpr UINT32 kVerticesCount = sizeof(vertices) / sizeof(vertices[0]);

	const TriangleIndex triangleIndices[]
	{
		{ 0, 1, 2 },
		{ 1, 3, 2 }
	};
	static constexpr UINT32 kIndicesCount = sizeof(triangleIndices) / sizeof(triangleIndices[0]);

	return drawIndexedPolygon(vertices, kVerticesCount, triangleIndices, kIndicesCount, pD3D11TextureView);
}

bool d3d11_renderer2d::CD3D11Renderer2D::drawIndexedPolygon(const Vertex2D* pVertices, UINT32 verticesCount, const TriangleIndex* pTriangleIndides, UINT32 indicesCount, ID3D11ShaderResourceView* pD3D11TextureView)
{
	if (!hasBeenInitialised())return false;

	if (!checkVertexBufferCapacity(verticesCount))return false;
	if (!checkIndexBufferCapacity(indicesCount))return false;

	HRESULT hr = E_FAIL;

	/* Vertices */
	{
		D3D11_MAPPED_SUBRESOURCE mappedVertices;
		hr = m_pStoredD3d11DeviceContext->Map(m_pVertexBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &mappedVertices);
		if (FAILED(hr))return false;

		if (mappedVertices.pData != nullptr)
		{
			::memcpy(mappedVertices.pData, pVertices, verticesCount * sizeof(Vertex2D));
		}

		m_pStoredD3d11DeviceContext->Unmap(m_pVertexBuffer, 0);
	}

	/* Indices*/
	{
		D3D11_MAPPED_SUBRESOURCE mappedIndices;
		hr = m_pStoredD3d11DeviceContext->Map(m_pIndexBuffer, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &mappedIndices);
		if (FAILED(hr))return false;

		if (mappedIndices.pData != nullptr)
		{
			::memcpy(mappedIndices.pData, pTriangleIndides, indicesCount * sizeof(TriangleIndex));
		}

		m_pStoredD3d11DeviceContext->Unmap(m_pIndexBuffer, 0);
	}

	UINT vertexStride = sizeof(Vertex2D);
	UINT vertexOffset = 0;
	static constexpr DXGI_FORMAT s_indexDxGiFormat = sizeof(TriangleIndex::i0) == 2U ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

	m_pStoredD3d11DeviceContext->IASetVertexBuffers(0, 1, &m_pVertexBuffer, &vertexStride, &vertexOffset);
	m_pStoredD3d11DeviceContext->IASetIndexBuffer(m_pIndexBuffer, s_indexDxGiFormat, 0);

	m_pStoredD3d11DeviceContext->PSSetShaderResources(0, 1, &pD3D11TextureView);

	m_pStoredD3d11DeviceContext->DrawIndexed(indicesCount * 3, 0, 0);

	return true;
}

bool d3d11_renderer2d::CD3D11Renderer2D::createCustomBlendMode(BlendFactor srcRGBA, BlendFactor dstRGBA, BlendOperation opRGBA) noexcept
{
	if (m_pStoredD3d11Device == nullptr)return false;

	ID3D11BlendState*& pCustomBlendState = m_pBlendStates[static_cast<size_t>(BlendMode::Custom)];
	if (pCustomBlendState != nullptr)
	{
		pCustomBlendState->Release();
		pCustomBlendState = nullptr;
	}

	const D3D11_BLEND_DESC bledDesc
	{
		FALSE,
		FALSE,
		{
			{
				TRUE,
				static_cast<D3D11_BLEND>(srcRGBA),
				static_cast<D3D11_BLEND>(dstRGBA),
				static_cast<D3D11_BLEND_OP>(opRGBA),
				static_cast<D3D11_BLEND>(srcRGBA),
				static_cast<D3D11_BLEND>(dstRGBA),
				static_cast<D3D11_BLEND_OP>(opRGBA)
			}
		}
	};
	HRESULT hr = m_pStoredD3d11Device->CreateBlendState(&bledDesc, &pCustomBlendState);

	return SUCCEEDED(hr);
}

bool d3d11_renderer2d::CD3D11Renderer2D::createCustomSeparatedBlendMode(BlendFactor srcRGB, BlendFactor dstRGB, BlendOperation opRGB, BlendFactor srcA, BlendFactor dstA, BlendOperation opA) noexcept
{
	if (m_pStoredD3d11Device == nullptr)return false;

	ID3D11BlendState*& pCustomSeparatedBlendState = m_pBlendStates[static_cast<size_t>(BlendMode::CustomSeparated)];
	if (pCustomSeparatedBlendState != nullptr)
	{
		pCustomSeparatedBlendState->Release();
		pCustomSeparatedBlendState = nullptr;
	}

	const D3D11_BLEND_DESC bledDesc
	{
		FALSE,
		FALSE,
		{
			{
				TRUE,
				static_cast<D3D11_BLEND>(srcRGB),
				static_cast<D3D11_BLEND>(dstRGB),
				static_cast<D3D11_BLEND_OP>(opRGB),
				static_cast<D3D11_BLEND>(srcA),
				static_cast<D3D11_BLEND>(dstA),
				static_cast<D3D11_BLEND_OP>(opA)
			}
		}
	};
	HRESULT hr = m_pStoredD3d11Device->CreateBlendState(&bledDesc, &pCustomSeparatedBlendState);

	return SUCCEEDED(hr);
}

void d3d11_renderer2d::CD3D11Renderer2D::releaseAll() noexcept
{
	if (m_pVertexShader2D != nullptr)
	{
		m_pVertexShader2D->Release();
		m_pVertexShader2D = nullptr;
	}
	if (m_pVertexInputLayout2D != nullptr)
	{
		m_pVertexInputLayout2D->Release();
		m_pVertexInputLayout2D = nullptr;
	}
	if (m_pVertexConstantBuffer2D != nullptr)
	{
		m_pVertexConstantBuffer2D->Release();
		m_pVertexConstantBuffer2D = nullptr;
	}

	if (m_pPixelShader2D != nullptr)
	{
		m_pPixelShader2D->Release();
		m_pPixelShader2D = nullptr;
	}

	releaseVertexBuffer();
	releaseIndexBuffer();

	if (m_pRasterizerState != nullptr)
	{
		m_pRasterizerState->Release();
		m_pRasterizerState = nullptr;
	}
	if (m_pDepthStencilState != nullptr)
	{
		m_pDepthStencilState->Release();
		m_pDepthStencilState = nullptr;
	}

	for (auto& pBlendState : m_pBlendStates)
	{
		if (pBlendState != nullptr)
		{
			pBlendState->Release();
			pBlendState = nullptr;
		}
	}

	for (auto& pSamplerState : m_pSamplterStates)
	{
		if (pSamplerState != nullptr)
		{
			pSamplerState->Release();
			pSamplerState = nullptr;
		}
	}
}

bool d3d11_renderer2d::CD3D11Renderer2D::createPredefinedBlendModes() noexcept
{
	struct PredefinedBlendDesc
	{
		BlendMode blendMode;
		D3D11_RENDER_TARGET_BLEND_DESC renderTargetBlendDesc;
	};
	constexpr const PredefinedBlendDesc s_predefinedBlendDescs[]
	{
		{
			BlendMode::NoBlend,
			{
				TRUE,
				D3D11_BLEND::D3D11_BLEND_SRC_ALPHA, D3D11_BLEND::D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
				D3D11_BLEND::D3D11_BLEND_ONE, D3D11_BLEND::D3D11_BLEND_ZERO, D3D11_BLEND_OP::D3D11_BLEND_OP_ADD,
				D3D11_COLOR_WRITE_ENABLE::D3D11_COLOR_WRITE_ENABLE_ALL
			}
		},
		{
			BlendMode::Spine_Normal,
			D3D11SpineBlendMode::Normal
		},
		{
			BlendMode::Spine_NormalPma,
			D3D11SpineBlendMode::NormalPma
		},
		{
			BlendMode::Spine_Add,
			D3D11SpineBlendMode::Add
		},
		{
			BlendMode::Spine_AddPma,
			D3D11SpineBlendMode::AddPma
		},
		{
			BlendMode::Spine_Multiply,
			D3D11SpineBlendMode::Multiply
		},
		{
			BlendMode::Spine_Screen,
			D3D11SpineBlendMode::Screen
		}
	};
	static constexpr size_t kPredefinedBlendModeCount = sizeof(s_predefinedBlendDescs) / sizeof(s_predefinedBlendDescs[0]);
	/* There are two custom blend-modes. */
	static_assert(kPredefinedBlendModeCount == kBlendStateCount - 2ULL, "The number of predefined blend-modes incorrect");

	bool hasCreatedAll = true;
	for (size_t i = 0; i < kPredefinedBlendModeCount; ++i)
	{
		const D3D11_RENDER_TARGET_BLEND_DESC& renderTargetBlendDesc = s_predefinedBlendDescs[i].renderTargetBlendDesc;
		const D3D11_BLEND_DESC bledDesc
		{
			FALSE,
			FALSE,
			renderTargetBlendDesc
		};

		HRESULT hr = m_pStoredD3d11Device->CreateBlendState(&bledDesc, &m_pBlendStates[i]);
		hasCreatedAll &= SUCCEEDED(hr);
		if (FAILED(hr))break;
	}

	return hasCreatedAll;
}

bool d3d11_renderer2d::CD3D11Renderer2D::createPredefinedSamplerStates() noexcept
{
	struct PredefinedSamplerStateDesc
	{
		SamplerState samplerState;
		D3D11_SAMPLER_DESC samplerDesc;
	};
	constexpr const PredefinedSamplerStateDesc s_predefinedSamplerStateDescs[]
	{
		{
			SamplerState::Bilenear_Clamp,
			{
				D3D11_FILTER_MIN_MAG_MIP_LINEAR,
				D3D11_TEXTURE_ADDRESS_CLAMP,
				D3D11_TEXTURE_ADDRESS_CLAMP,
				D3D11_TEXTURE_ADDRESS_CLAMP,
				0.f,
				1,
				D3D11_COMPARISON_NEVER,
				{ 1.f, 1.f, 1.f, 1.f },
				-D3D11_FLOAT32_MAX,
				D3D11_FLOAT32_MAX
			}
		},
		{
			SamplerState::Nearest_Clamp,
			{
				D3D11_FILTER_MIN_MAG_MIP_POINT,
				D3D11_TEXTURE_ADDRESS_CLAMP,
				D3D11_TEXTURE_ADDRESS_CLAMP,
				D3D11_TEXTURE_ADDRESS_CLAMP,
				0.f,
				1,
				D3D11_COMPARISON_NEVER,
				{ 1.f, 1.f, 1.f, 1.f },
				-D3D11_FLOAT32_MAX,
				D3D11_FLOAT32_MAX
			}
		},
	};
	static constexpr size_t kPredefinedSamplerStateCount = sizeof(s_predefinedSamplerStateDescs) / sizeof(s_predefinedSamplerStateDescs[0]);
	static_assert(kPredefinedSamplerStateCount == kSamplerStateCount, "The number of predefined sampler states incorrect");
	bool hasCreatedAll = true;
	for (size_t i = 0; i < kPredefinedSamplerStateCount; ++i)
	{
		const D3D11_SAMPLER_DESC samplerDesc = s_predefinedSamplerStateDescs[i].samplerDesc;

		HRESULT hr = m_pStoredD3d11Device->CreateSamplerState(&samplerDesc, &m_pSamplterStates[i]);
		hasCreatedAll &= SUCCEEDED(hr);
	}

	return hasCreatedAll;
}

void d3d11_renderer2d::CD3D11Renderer2D::releaseVertexBuffer() noexcept
{
	if (m_pVertexBuffer != nullptr)
	{
		m_pVertexBuffer->Release();
		m_pVertexBuffer = nullptr;
	};

	m_nVertexBufferCapacity = 0;
}

bool d3d11_renderer2d::CD3D11Renderer2D::checkVertexBufferCapacity(UINT32 vertexCount) noexcept
{
	if (m_pVertexBuffer == nullptr)
	{
		return recreateVertexBuffer(vertexCount);
	}
	else if (m_nVertexBufferCapacity < vertexCount)
	{
		return recreateVertexBuffer(vertexCount);
	}

	return true;
}

bool d3d11_renderer2d::CD3D11Renderer2D::recreateVertexBuffer(UINT32 verticesCount) noexcept
{
	if (m_pStoredD3d11Device == nullptr)return false;

	releaseVertexBuffer();
	const D3D11_BUFFER_DESC bufferDesc
	{
		verticesCount * sizeof(Vertex2D),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_VERTEX_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
		0,
		0
	};
	HRESULT hr = m_pStoredD3d11Device->CreateBuffer(&bufferDesc, nullptr, &m_pVertexBuffer);
	if (SUCCEEDED(hr))
	{
		m_nVertexBufferCapacity = verticesCount;
	}

	return SUCCEEDED(hr);
}

void d3d11_renderer2d::CD3D11Renderer2D::releaseIndexBuffer() noexcept
{
	if (m_pIndexBuffer != nullptr)
	{
		m_pIndexBuffer->Release();
		m_pIndexBuffer = nullptr;
	};

	m_nIndexBufferCapacity = 0;
}

bool d3d11_renderer2d::CD3D11Renderer2D::checkIndexBufferCapacity(UINT32 indiceCount) noexcept
{
	if (m_pIndexBuffer == nullptr)
	{
		return recreateIndexBuffer(indiceCount);
	}
	else if (m_nIndexBufferCapacity < indiceCount)
	{
		return recreateIndexBuffer(indiceCount);
	}

	return true;
}

bool d3d11_renderer2d::CD3D11Renderer2D::recreateIndexBuffer(UINT32 indicesCount) noexcept
{
	if (m_pStoredD3d11Device == nullptr)return false;

	releaseIndexBuffer();
	const D3D11_BUFFER_DESC bufferDesc
	{
		/* 取り敢えず三角形のみ */
		indicesCount * sizeof(TriangleIndex),
		D3D11_USAGE_DYNAMIC,
		D3D11_BIND_INDEX_BUFFER,
		D3D11_CPU_ACCESS_WRITE,
		0,
		0
	};
	HRESULT hr = m_pStoredD3d11Device->CreateBuffer(&bufferDesc, nullptr, &m_pIndexBuffer);
	if (SUCCEEDED(hr))
	{
		m_nIndexBufferCapacity = indicesCount;
	}

	return SUCCEEDED(hr);
}
