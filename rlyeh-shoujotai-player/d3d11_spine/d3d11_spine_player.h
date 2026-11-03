#ifndef D3D11_SPINE_PLAYER_H_
#define D3D11_SPINE_PLAYER_H_

#include "spine_player.h"

class CD3D11SpinePlayer : public CSpinePlayer
{
public:
	CD3D11SpinePlayer(ID3D11Device* pD3D11Device, HWND hRenderTargetWindow);
	virtual ~CD3D11SpinePlayer() = default;

	void draw(float fScale = 1.f, float fOffsetX = 0.f, float fOffsetY = 0.f);

	void calculateTransformValues(UINT renderTaregtWidth, UINT renderTargetHeight, float& fScale, float& fOffsetX, float& fOffsetY) const noexcept;
	D2D_VECTOR_4F getCurrentBoundingBox() const;
	D2D_VECTOR_4F getCurrentBoundingBoxOfSlot(const char* slotName, size_t nameLength) const;
	template<size_t nameSize>
	D2D_VECTOR_4F getCurrentBoundingBoxOfSlot(const char(&slotName)[nameSize]) const
	{
		return getCurrentBoundingBoxOfSlot(slotName, nameSize - 1);
	}
private:
	HWND m_hRendertargetWindow = nullptr;

	void workOutDefaultScale() override;
	bool workOutDefaultSizeAndOffset() override;
};

#endif // !D3D11_SPINE_PLAYER_H_
