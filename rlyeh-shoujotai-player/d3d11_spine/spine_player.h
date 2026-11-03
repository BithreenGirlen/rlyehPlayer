#ifndef SPINE_PLAYER_H_
#define SPINE_PLAYER_H_

/* Base-type spine player regardless of rendering library. */

#include <string>
#include <vector>
#include <unordered_map>

#include "d3d11_spine_drawable.h"
#include "d3d11_spine_texture_loader.h"

using FPoint2 = D2D_POINT_2F;
using CSpineDrawable = CD3D11SpineDrawable;
using CTextureLoader = CD3D11SpineTextureLoader;


class CSpinePlayer
{
public:
	CSpinePlayer() = default;
	virtual ~CSpinePlayer() = default;

	void clearDrawables();

	bool loadSpineFromFile(const char* atlasFilePath, const char* skeletonFilePath, bool isBinarySkel, void* pRendererObject);
	bool loadSpineFromMemory(const char* atlasFileData, size_t atlasFileDataLength, const char* textureDirectory, const char* skeletonFileData, size_t skeletonFileDataLength, bool isBinarySkel, void* pRendererObject);

	bool addDrawable(SpineUniquePtr<CSpineDrawable>& pDrawable);
	/// @brief Pass like CSpinePlayer::getDrawables[0]->duplicate();
	bool addDrawable(SpineUniquePtr<CSpineDrawable>&& pDrawable);

	size_t getNumberOfSpines() const noexcept;
	bool hasSpineBeenLoaded() const noexcept;
	/// @brief Intended to be used to duplicate drawable and to add to another Spine player.
	const std::vector<SpineUniquePtr<CSpineDrawable>>& getDrawables() const noexcept;

	void update(float fDelta);

	void resetScale();

	void addOffset(int iX, int iY);

	void shiftAnimation();
	void shiftSkin();

	void setAnimationByIndex(size_t nIndex);
	void setAnimationByName(const char* szAnimationName);
	void restartAnimation(bool loop = true);

	void setSkinByIndex(size_t nIndex);
	void setSkinByName(const char* szSkinName);
	void setupSkin();

	/// @brief To perform PMA blend-mode on the colours of vertices or not.
	/// @return false if it were out of range.
	bool premultiplyAlpha(bool premultiplied, size_t nDrawableIndex = 0) noexcept;
	void premultiplyAlphaAll(bool premultiplied) noexcept;
	/// @return current state. If it were out of range, return false.
	bool isAlphaPremultiplied(size_t nDrawableIndex = 0) const noexcept;

	/// @brief To force blend-mode-normal or not.
	/// @return false if it were out of range
	bool forceBlendModeNormal(bool toForce, size_t nDrawableIndex = 0) noexcept;
	void forceBlendModeNormalAll(bool toForce) noexcept;
	bool isBlendModeNormalForced(size_t nDrawableIndex = 0) const noexcept;

	bool setPause(bool paused, size_t nDrawableIndex = 0) noexcept;
	void setPauseAll(bool paused) noexcept;
	bool isPaused(size_t nDrawableIndex = 0) const noexcept;

	bool setVisibility(bool visible, size_t nDrawableIndex = 0) noexcept;
	void setVisibilityAll(bool visible) noexcept;
	bool isVisible(size_t nDrawableIndex = 0) const noexcept;

	bool setPhysics(CSpineDrawable::Physics physics, size_t nDrawableIndex = 0) noexcept;
	void setPhysicsAll(CSpineDrawable::Physics physics) noexcept;
	CSpineDrawable::Physics getPhysics(size_t nDrawableIndex = 0) const noexcept;

	void setDrawOrder(bool reversed) noexcept;
	bool isDrawOrderReversed() const noexcept;

	void enableConversionToPmaOnLoading(bool toEnable) noexcept;
	bool isConversionToPmaOnLoadingEnabled() const noexcept;
	void setTextureLoadCallback(void (*pFunc)(void* pUserDatum, const char* textureFilePath, size_t filePathLength, void* pOutImage), void* pUserDatum) noexcept;

	/// @brief Current animation name in track; maybe nullptr
	const char* getCurrentAnimationName();
	/// @brief Get animation time actually entried in track.
	/// @param fTrack elapsed time since the track was entried.
	/// @param fLast current timeline position.
	/// @param fStart timeline start position.
	/// @param fEnd timeline end position.
	void getCurrentAnimationTime(float* fTrack, float* fLast, float* fStart, float* fEnd);
	void setCurrentAnimationTime(float animationTime);
	float getAnimationDuration(const char* animationName);

	/// @brief Maybe nullptr
	const char* getCurrentSkinName();

	const std::vector<std::string>& getSlotNames() const noexcept;
	const std::vector<std::string>& getSkinNames() const noexcept;
	const std::vector<std::string>& getAnimationNames() const noexcept;

	void setSlotsToExclude(const std::vector<std::string>& slotNames);
	void mixSkins(const std::vector<std::string>& skinNames);
	void addAnimationTracks(const std::vector<std::string>& animationNames, bool loop = false);
	void mixAnimations(const char* fadeOutAnimationName, const char* fadeInAnimationName, float mixTime);
	void clearMixedAnimation();

	void setSlotExcludeCallback(bool (*pFunc)(const char*, size_t, void*), void* pUserDatum);

	/// @brief Searches slots having multiple attachments. If each slot is associated with only single attachment, returns empty.
	/// @return slot name as key and attachment names as values.
	std::unordered_map<std::string, std::vector<std::string>> getSlotNamesWithTheirAttachments();
	bool replaceAttachment(const char* szSlotName, const char* szAttachmentName);

	FPoint2 getBaseSize() const noexcept;
	void setBaseSize(float fWidth, float fHeight);
	void resetBaseSize();

	FPoint2 getOffset() const noexcept;
	void setOffset(float fX, float fY) noexcept;

	float getSkeletonScale() const noexcept;
	void setSkeletonScale(float fScale);

	float getCanvasScale() const noexcept;
	void setCanvasScale(float fScale) noexcept;

	float getTimeScale() const noexcept;
	void setTimeScale(float fTimeScale) noexcept;
protected:
	enum Constants { kBaseWidth = 1280, kBaseHeight = 720 };

	CTextureLoader m_textureLoader;
	std::vector<SpineUniquePtr<CSpineDrawable>> m_drawables;

	FPoint2 m_fBaseSize = FPoint2{ kBaseWidth, kBaseHeight };

	float m_fDefaultScale = 1.f;
	FPoint2 m_fDefaultOffset{};

	float m_fTimeScale = 1.f;
	float m_fSkeletonScale = 1.f;
	float m_fCanvasScale = 1.f;
	FPoint2 m_fOffset{};

	std::vector<std::string> m_animationNames;
	size_t m_nAnimationIndex = 0;

	std::vector<std::string> m_skinNames;
	size_t m_nSkinIndex = 0;

	std::vector<std::string> m_slotNames;

	bool m_isDrawOrderReversed = false;

	virtual void workOutDefaultScale() = 0;
	virtual bool workOutDefaultSizeAndOffset() = 0;

	void updatePosition();

	void clearAnimationTracks();
};

#endif // !SPINE_PLAYER_H_
