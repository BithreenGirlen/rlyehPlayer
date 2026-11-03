
#include <float.h> /* To calculate bounding-box. */

#include "d3d11_spine_drawable.h"

#if defined(SPINE_42)
	static_assert(static_cast<spine::Physics>(CD3D11SpineDrawable::Physics::None) == spine::Physics::Physics_None, "Physics_None");
	static_assert(static_cast<spine::Physics>(CD3D11SpineDrawable::Physics::Reset) == spine::Physics::Physics_Reset, "Physics_Reset");
	static_assert(static_cast<spine::Physics>(CD3D11SpineDrawable::Physics::Update) == spine::Physics::Physics_Update, "Physics_Update");
	static_assert(static_cast<spine::Physics>(CD3D11SpineDrawable::Physics::Pose) == spine::Physics::Physics_Pose, "Physics_Pose");
#endif


CD3D11SpineDrawable::CD3D11SpineDrawable(SpineSharedPtr<spine::Atlas> atlas, SpineSharedPtr<spine::SkeletonData> skeletonData, void* pRendererObject)
	:m_atlas(atlas), m_skeletonData(skeletonData), m_pRendererObject(pRendererObject)
{
	spine::Bone::setYDown(true);

	m_skeleton = SpineUniquePtr<spine::Skeleton>::MakeUnique(m_skeletonData.get());
	m_animationStateData = SpineUniquePtr<spine::AnimationStateData>::MakeUnique(m_skeletonData.get());
	m_animationState = SpineUniquePtr<spine::AnimationState>::MakeUnique(m_animationStateData.get());
}

SpineUniquePtr<CD3D11SpineDrawable> CD3D11SpineDrawable::duplicate()
{
	return SpineUniquePtr<CD3D11SpineDrawable>::MakeUnique(m_atlas, m_skeletonData, m_pRendererObject);
}

spine::Skeleton* CD3D11SpineDrawable::skeleton() const noexcept
{
	return m_skeleton.get();
}

spine::AnimationState* CD3D11SpineDrawable::animationState() const noexcept
{
	return m_animationState.get();
}

void CD3D11SpineDrawable::premultiplyAlpha(bool premultiplied) noexcept
{
	m_isAlphaPremultiplied = premultiplied;
}

bool CD3D11SpineDrawable::isAlphaPremultiplied() const noexcept
{
	return m_isAlphaPremultiplied;
}

void CD3D11SpineDrawable::forceBlendModeNormal(bool toForce) noexcept
{
	m_isToForceBlendModeNormal = toForce;
}

bool CD3D11SpineDrawable::isBlendModeNormalForced() const noexcept
{
	return m_isToForceBlendModeNormal;
}

void CD3D11SpineDrawable::setPause(bool paused) noexcept
{
	m_isPaused = paused;
}

bool CD3D11SpineDrawable::isPaused() const noexcept
{
	return m_isPaused;
}

void CD3D11SpineDrawable::setVisibility(bool visible) noexcept
{
	m_isVisible = visible;
}

bool CD3D11SpineDrawable::isVisible() const noexcept
{
	return m_isVisible;
}

void CD3D11SpineDrawable::setPhysics(Physics physics) noexcept
{
	m_physics = physics;
}

CD3D11SpineDrawable::Physics CD3D11SpineDrawable::getPhysics() const noexcept
{
	return m_physics;
}

void CD3D11SpineDrawable::update(float fDelta)
{
	if (m_skeleton != nullptr && m_animationState != nullptr)
	{
		if (!m_isPaused)m_animationState->update(fDelta);
		m_animationState->apply(*m_skeleton);

		/* Spine 4.1 does not have Skeleton::update() */
#if !defined(SPINE_41)
		if (!m_isPaused)m_skeleton->update(fDelta);
#endif

#if defined(SPINE_42)
		m_skeleton->updateWorldTransform(static_cast<spine::Physics>(m_physics));
#else
		m_skeleton->updateWorldTransform();
#endif
	}
}

void CD3D11SpineDrawable::draw(float fScale, float fOffsetX, float fOffsetY)
{
	if (!m_isVisible)return;
	if (m_skeleton == nullptr || m_animationState == nullptr)return;
	if (m_skeleton->getColor().a == 0)return;

	auto* pRenderer = static_cast<d3d11_renderer2d::CD3D11Renderer2D*>(m_pRendererObject);
	if (pRenderer == nullptr)return;

	static unsigned short s_quadIndices[] = { 0, 1, 2, 2, 3, 0 };

	for (size_t i = 0; i < m_skeleton->getSlots().size(); ++i)
	{
		spine::Slot& slot = *m_skeleton->getDrawOrder()[i];
		spine::Attachment* pAttachment = slot.getAttachment();

		if (pAttachment == nullptr || slot.getColor().a == 0 || !slot.getBone().isActive())
		{
			m_clipper.clipEnd(slot);
			continue;
		}

		if (isToBeLeftOut(slot.getData().getName()))
		{
			m_clipper.clipEnd(slot);
			continue;
		}

		spine::Vector<float>* pVertices = &m_worldVertices;
		spine::Vector<float>* pAttachmentUvs = nullptr;
		unsigned short* pIndices = nullptr;
		size_t nIndicesCount = 0;

		spine::Color* pAttachmentColor = nullptr;

		ID3D11ShaderResourceView* pD3D11TextureView = nullptr;

		if (pAttachment->getRTTI().isExactly(spine::RegionAttachment::rtti))
		{
			spine::RegionAttachment* pRegionAttachment = (spine::RegionAttachment*)pAttachment;
			pAttachmentColor = &pRegionAttachment->getColor();

			if (pAttachmentColor->a == 0)
			{
				m_clipper.clipEnd(slot);
				continue;
			}

			m_worldVertices.setSize(8, 0);
#if defined (SPINE_41) || defined (SPINE_42)
			pRegionAttachment->computeWorldVertices(slot, m_worldVertices, 0, 2);
#else
			pRegionAttachment->computeWorldVertices(slot.getBone(), m_worldVertices, 0, 2);
#endif
			pAttachmentUvs = &pRegionAttachment->getUVs();
			pIndices = s_quadIndices;
			nIndicesCount = sizeof(s_quadIndices) / sizeof(unsigned short);

#if defined (SPINE_41) || defined (SPINE_42)
			spine::AtlasRegion* pAtlasRegion = static_cast<spine::AtlasRegion*>(pRegionAttachment->getRegion());
			m_isAlphaPremultiplied = pAtlasRegion->page->pma;
			pD3D11TextureView = reinterpret_cast<ID3D11ShaderResourceView*>(pAtlasRegion->page->texture);
#else
			spine::AtlasRegion* pAtlasRegion = static_cast<spine::AtlasRegion*>(pRegionAttachment->getRendererObject());
#ifdef SPINE_40
			m_isAlphaPremultiplied = pAtlasRegion->page->pma;
#endif
			pD3D11TextureView = reinterpret_cast<ID3D11ShaderResourceView*>(pAtlasRegion->page->getRendererObject());
#endif
		}
		else if (pAttachment->getRTTI().isExactly(spine::MeshAttachment::rtti))
		{
			spine::MeshAttachment* pMeshAttachment = (spine::MeshAttachment*)pAttachment;
			pAttachmentColor = &pMeshAttachment->getColor();

			if (pAttachmentColor->a == 0)
			{
				m_clipper.clipEnd(slot);
				continue;
			}
			m_worldVertices.setSize(pMeshAttachment->getWorldVerticesLength(), 0);
			pMeshAttachment->computeWorldVertices(slot, 0, pMeshAttachment->getWorldVerticesLength(), m_worldVertices, 0, 2);
			pAttachmentUvs = &pMeshAttachment->getUVs();
			pIndices = pMeshAttachment->getTriangles().buffer();
			nIndicesCount = pMeshAttachment->getTriangles().size();

#if defined (SPINE_41) || defined (SPINE_42)
			spine::AtlasRegion* pAtlasRegion = static_cast<spine::AtlasRegion*>(pMeshAttachment->getRegion());
			m_isAlphaPremultiplied = pAtlasRegion->page->pma;
			pD3D11TextureView = reinterpret_cast<ID3D11ShaderResourceView*>(pAtlasRegion->page->texture);
#else
			spine::AtlasRegion* pAtlasRegion = static_cast<spine::AtlasRegion*>(pMeshAttachment->getRendererObject());
#ifdef SPINE_40
			m_isAlphaPremultiplied = pAtlasRegion->page->pma;
#endif
			pD3D11TextureView = reinterpret_cast<ID3D11ShaderResourceView*>(pAtlasRegion->page->getRendererObject());
#endif
		}
		else if (pAttachment->getRTTI().isExactly(spine::ClippingAttachment::rtti))
		{
			spine::ClippingAttachment* clip = (spine::ClippingAttachment*)slot.getAttachment();
			m_clipper.clipStart(slot, clip);
			continue;
		}
		else
		{
			m_clipper.clipEnd(slot);
			continue;
		}

		if (m_clipper.isClipping())
		{
			m_clipper.clipTriangles(m_worldVertices.buffer(), pIndices, nIndicesCount, pAttachmentUvs->buffer(), 2);
			if (m_clipper.getClippedTriangles().size() == 0)
			{
				m_clipper.clipEnd(slot);
				continue;
			}
			pVertices = &m_clipper.getClippedVertices();
			pAttachmentUvs = &m_clipper.getClippedUVs();
			pIndices = m_clipper.getClippedTriangles().buffer();
			nIndicesCount = m_clipper.getClippedTriangles().size();
		}

		const spine::Color& skeletonColor = m_skeleton->getColor();
		const spine::Color& slotColor = slot.getColor();
		const spine::Color spineColor
		(
			skeletonColor.r * slotColor.r * pAttachmentColor->r,
			skeletonColor.g * slotColor.g * pAttachmentColor->g,
			skeletonColor.b * slotColor.b * pAttachmentColor->b,
			skeletonColor.a * slotColor.a * pAttachmentColor->a
		);
		const D2D_VECTOR_4F d3d11Color
		{
			spineColor.r * (m_isAlphaPremultiplied ? spineColor.a : 1.f),
			spineColor.g * (m_isAlphaPremultiplied ? spineColor.a : 1.f),
			spineColor.b * (m_isAlphaPremultiplied ? spineColor.a : 1.f),
			spineColor.a
		};

		m_vertices2D.setSize(pVertices->size() / 2, {});
		for (size_t ii = 0, k = 0; ii < pVertices->size() - 1ULL; ii += 2, ++k)
		{
			d3d11_renderer2d::Vertex2D& vertex2D = m_vertices2D[k];

			vertex2D.pos.x = (*pVertices)[ii] * fScale + fOffsetX;
			vertex2D.pos.y = (*pVertices)[ii + 1LL] * fScale + fOffsetY;

			vertex2D.colour = d3d11Color;

			vertex2D.tex.x = (*pAttachmentUvs)[ii];
			vertex2D.tex.y = (*pAttachmentUvs)[ii + 1LL];
		}

		m_triangleIndices.setSize(nIndicesCount / 3, {});
		for (size_t ii = 0, k = 0; ii < nIndicesCount - 2ULL; ii += 3, ++k)
		{
			auto& triangleIndex = m_triangleIndices[k];
			triangleIndex.i0 = static_cast<UINT16>(pIndices[ii]);
			triangleIndex.i1 = static_cast<UINT16>(pIndices[ii + 1ULL]);
			triangleIndex.i2 = static_cast<UINT16>(pIndices[ii + 2ULL]);
		}

		d3d11_renderer2d::BlendMode d3d11BlendMode;
		const spine::BlendMode spineBlendMode = m_isToForceBlendModeNormal ? spine::BlendMode::BlendMode_Normal : slot.getData().getBlendMode();
		switch (spineBlendMode)
		{
		case spine::BlendMode_Additive:
			d3d11BlendMode = m_isAlphaPremultiplied ? d3d11_renderer2d::BlendMode::Spine_AddPma : d3d11_renderer2d::BlendMode::Spine_Add;
			break;
		case spine::BlendMode_Multiply:
			d3d11BlendMode = d3d11_renderer2d::BlendMode::Spine_Multiply;
			break;
		case spine::BlendMode_Screen:
			d3d11BlendMode = d3d11_renderer2d::BlendMode::Spine_Screen;
			break;
		default:
			d3d11BlendMode = m_isAlphaPremultiplied ? d3d11_renderer2d::BlendMode::Spine_NormalPma : d3d11_renderer2d::BlendMode::Spine_Normal;
			break;
		}

		pRenderer->setBlendMode(d3d11BlendMode);
		pRenderer->drawIndexedPolygon(
			m_vertices2D.buffer(),
			static_cast<UINT32>(m_vertices2D.size()),
			m_triangleIndices.buffer(),
			static_cast<UINT32>(m_triangleIndices.size()),
			pD3D11TextureView);

		m_clipper.clipEnd(slot);
	}
	m_clipper.clipEnd();

	pRenderer->setBlendMode(d3d11_renderer2d::BlendMode::NoBlend);
}

void CD3D11SpineDrawable::setLeaveOutList(spine::Vector<spine::String>& list) noexcept
{
	m_leaveOutList.clearAndAddAll(list);
}

void CD3D11SpineDrawable::setLeaveOutCallback(LeaveOutCallback pFunc, void* pUserDatum) noexcept
{
	m_pLeaveOutCallback = pFunc;
	m_pLeaveOutCallackUserDatum = pUserDatum;
}

D2D_VECTOR_4F CD3D11SpineDrawable::getBoundingBox() noexcept
{
	/* D2D_RECT_F cannot be adopted here because its the style is bottom/right, not width/height. */
	D2D_VECTOR_4F boundingBox{};

	if (m_skeleton != nullptr)
	{
		m_skeleton->getBounds(boundingBox.x, boundingBox.y, boundingBox.z, boundingBox.w, m_tempVertices);
	}

	return boundingBox;
}

D2D_VECTOR_4F CD3D11SpineDrawable::getBoundingBoxOfSlot(const char* slotName, size_t nameLength, bool* found) noexcept
{
	float fMinX = FLT_MAX;
	float fMinY = FLT_MAX;
	float fMaxX = -FLT_MAX;
	float fMaxY = -FLT_MAX;

	if (m_skeleton != nullptr)
	{
		for (size_t i = 0; i < m_skeleton->getSlots().size(); ++i)
		{
			spine::Slot& slot = *m_skeleton->getDrawOrder()[i];
			const spine::String& slotDataName = slot.getData().getName();
			if (nameLength != slotDataName.length())continue;

			if (::memcmp(slotDataName.buffer(), slotName, slotDataName.length()) == 0)
			{
				spine::Attachment* pAttachment = slot.getAttachment();
				if (pAttachment != nullptr)
				{
					if (pAttachment->getRTTI().isExactly(spine::RegionAttachment::rtti))
					{
						spine::RegionAttachment* pRegionAttachment = static_cast<spine::RegionAttachment*>(pAttachment);

						m_tempVertices.setSize(8, 0);

#if defined (SPINE_41) || defined (SPINE_42)
						pRegionAttachment->computeWorldVertices(slot, m_tempVertices, 0, 2);
#else
						pRegionAttachment->computeWorldVertices(slot.getBone(), m_tempVertices, 0, 2);
#endif
					}
					else if (pAttachment->getRTTI().isExactly(spine::MeshAttachment::rtti))
					{
						spine::MeshAttachment* pMeshAttachment = static_cast<spine::MeshAttachment*>(pAttachment);

						m_tempVertices.setSize(pMeshAttachment->getWorldVerticesLength(), 0);

						pMeshAttachment->computeWorldVertices(slot, 0, pMeshAttachment->getWorldVerticesLength(), m_tempVertices, 0, 2);
					}
					else
					{
						continue;
					}

					for (size_t ii = 0; ii < m_tempVertices.size(); ii += 2)
					{
						float fX = m_tempVertices[ii];
						float fY = m_tempVertices[ii + 1LL];

						fMinX = fMinX < fX ? fMinX : fX;
						fMinY = fMinY < fY ? fMinY : fY;
						fMaxX = fMaxX > fX ? fMaxX : fX;
						fMaxY = fMaxY > fY ? fMaxY : fY;
					}

					if (found != nullptr)*found = true;
					break;
				}
			}
		}
	}

	return D2D_VECTOR_4F{ fMinX, fMinY, fMaxX - fMinX, fMaxY - fMinY };
}

bool CD3D11SpineDrawable::isToBeLeftOut(const spine::String& slotName) noexcept
{
	if (m_pLeaveOutCallback != nullptr)
	{
		return m_pLeaveOutCallback(slotName.buffer(), slotName.length(), m_pLeaveOutCallackUserDatum);
	}
	else
	{
		return m_leaveOutList.contains(slotName);
	}
}
