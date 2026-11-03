

#include "d3d11_spine_player.h"


CD3D11SpinePlayer::CD3D11SpinePlayer(ID3D11Device* pD3D11Device, HWND hRenderTargetWindow)
	:m_hRendertargetWindow(hRenderTargetWindow)
{
	m_textureLoader.setD3D11Device(pD3D11Device);
}

void CD3D11SpinePlayer::draw(float fScale, float fOffsetX, float fOffsetY)
{
	if (!m_drawables.empty())
	{
		if (!m_isDrawOrderReversed)
		{
			for (size_t i = 0; i < m_drawables.size(); ++i)
			{
				m_drawables[i]->draw(fScale, fOffsetX, fOffsetY);
			}
		}
		else
		{
			for (long long i = m_drawables.size() - 1; i >= 0; --i)
			{
				m_drawables[i]->draw(fScale, fOffsetX, fOffsetY);
			}
		}
	}
}

void CD3D11SpinePlayer::calculateTransformValues(UINT renderTaregtWidth, UINT renderTargetHeight, float& fScale, float& fOffsetX, float& fOffsetY) const noexcept
{
	fScale = m_fSkeletonScale;
	fOffsetX = (renderTaregtWidth - m_fBaseSize.x * m_fSkeletonScale) / 2.f;
	fOffsetY = (renderTargetHeight - m_fBaseSize.y * m_fSkeletonScale) / 2.f;
}

D2D_VECTOR_4F CD3D11SpinePlayer::getCurrentBoundingBox() const
{
	float fMinX = FLT_MAX;
	float fMinY = FLT_MAX;
	float fMaxX = -FLT_MAX;
	float fMaxY = -FLT_MAX;

	for (const auto& drawable : m_drawables)
	{
		const D2D_VECTOR_4F rect = drawable->getBoundingBox();
		fMinX = (std::min)(fMinX, rect.x);
		fMinY = (std::min)(fMinY, rect.y);
		fMaxX = (std::max)(fMaxX, rect.x + rect.z);
		fMaxY = (std::max)(fMaxY, rect.y + rect.w);
	}

	return { fMinX, fMinY, fMaxX - fMinX, fMaxY - fMinY };
}

D2D_VECTOR_4F CD3D11SpinePlayer::getCurrentBoundingBoxOfSlot(const char* slotName, size_t nameLength) const
{
	bool found = false;
	for (const auto& drawable : m_drawables)
	{
		D2D_VECTOR_4F rect = drawable->getBoundingBoxOfSlot(slotName, nameLength, &found);
		if (found)
		{
			return rect;
		}
	}

	return {};
}

void CD3D11SpinePlayer::workOutDefaultScale()
{
	m_fDefaultScale = 1.f;

	const int skeletonWidth = static_cast<int>(m_fBaseSize.x);
	const int skeletonHeight = static_cast<int>(m_fBaseSize.y);

	int monitorWidth = 0;
	int monitorHeight = 0;

	HMONITOR hMonitor = ::MonitorFromWindow(m_hRendertargetWindow, MONITOR_DEFAULTTONEAREST);
	if (hMonitor != nullptr)
	{
		MONITORINFO monitorInfo{ sizeof(MONITORINFO) };
		BOOL iRet = ::GetMonitorInfoW(hMonitor, &monitorInfo);
		if (iRet)
		{
			monitorWidth = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
			monitorHeight = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
		}
	}

	if (monitorWidth == 0 && monitorHeight == 0) return;

	if (skeletonWidth > monitorWidth || skeletonHeight > monitorHeight)
	{
		const float fScaleX = static_cast<float>(monitorWidth) / skeletonWidth;
		const float fScaleY = static_cast<float>(monitorHeight) / skeletonHeight;

		m_fDefaultScale = fScaleX > fScaleY ? fScaleY : fScaleX;
	}
}

bool CD3D11SpinePlayer::workOutDefaultSizeAndOffset()
{
	static constexpr float kMaxDimension = 16384.f;

	float fMinX = FLT_MAX;
	float fMinY = FLT_MAX;
	float fWidth = -FLT_MAX;
	float fHeight = -FLT_MAX;

	for (const auto& pDrawable : m_drawables)
	{
		const auto& rect = pDrawable->getBoundingBox();
		fMinX = (std::min)(fMinX, rect.x);
		fMinY = (std::min)(fMinY, rect.y);

		if (::isless(rect.z, kMaxDimension) && ::isless(rect.w, kMaxDimension))
		{
			fWidth = (std::max)(fWidth, rect.z);
			fHeight = (std::max)(fHeight, rect.w);
		}
	}

	if (::isless(fMinX, FLT_MAX) && ::isless(fMinY, FLT_MAX))
	{
		m_fDefaultOffset.x = fMinX;
		m_fDefaultOffset.y = fMinY;
	}

	if (::isgreater(fWidth, -FLT_MAX) && ::isgreater(fHeight, -FLT_MAX))
	{
		m_fBaseSize.x = fWidth;
		m_fBaseSize.y = fHeight;

		return true;
	}

	return false;
}
