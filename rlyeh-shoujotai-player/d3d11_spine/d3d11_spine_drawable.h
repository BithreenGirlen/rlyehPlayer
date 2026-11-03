#ifndef D3D11_SPINE_DRAWABLE_H_
#define D3D11_SPINE_DRAWABLE_H_

#include <d3d11.h>
#include <dcommon.h> /* To return bounding-box as D2D_VECTOR_4F. */

#include "spine_shared_ptr.h"
#include "spine_unique_ptr.h"

#include "../d3d11_renderer2d.h"

#include <spine/spine.h>

class CD3D11SpineDrawable
{
public:
	explicit CD3D11SpineDrawable(SpineSharedPtr<spine::Atlas> atlas, SpineSharedPtr<spine::SkeletonData> skeletonData, void* pRendererObject);
	~CD3D11SpineDrawable() = default;

	/// @brief 物理演算の更新方法と適用方法
	enum class Physics : unsigned char
	{
		None = 0, /* 更新も適用も行わない。 */
		Reset, /* 通算の適用をリセットし、新たに物理演算を行う。 */
		Update, /* 更新し、適用も行う。 */
		Pose /* 適用するが、更新は行わない。 */
	};

	/// @brief 描画対象のスロットを実際に描画するか否かを判断するコールバック関数。
	/// @param slotName UTF-8形式のスロット名
	/// @param slotNameLength スロット名の長さ
	/// @param pUserDatum コールバック関数登録時に渡された任意データ。
	/// @return Return 除外すべきスロットの場合true、描画を行う場合false
	typedef bool(*LeaveOutCallback)(const char* slotName, size_t slotNameLength, void* pUserDatum);

	/// @brief Atlas並びSkeletonDataを共有して複製
	[[nodiscard]] SpineUniquePtr<CD3D11SpineDrawable> duplicate();

	spine::Skeleton* skeleton() const noexcept;
	spine::AnimationState* animationState() const noexcept;

	/// @brief 乗算済みαの混色法を適用するか否か。Spine3.8以前でのみ有効
	void premultiplyAlpha(bool premultiplied) noexcept;
	bool isAlphaPremultiplied() const noexcept;

	/// @brief スロット指定の混色法を無視して通常混色法を適用するか否か。デバッグ用途
	void forceBlendModeNormal(bool toForce) noexcept;
	bool isBlendModeNormalForced() const noexcept;

	/// @brief 時間更新を行うか否か
	void setPause(bool paused) noexcept;
	bool isPaused() const noexcept;

	/// @brief 描画を行うか否か
	void setVisibility(bool visible) noexcept;
	bool isVisible() const noexcept;

	/// @brief 物理演算法指定。Spine4.2以降でのみ有効
	void setPhysics(Physics physics) noexcept;
	Physics getPhysics() const noexcept;

	/// @brief 時間の加算並びワールド座標の更新
	/// @param fDelta 加算すべき時間(秒単位)
	/// @remark 停止中や0.0fの時間加算であっても座標更新は行う
	void update(float fDelta);
	/// @brief 描画。変形行列を用いずに拡縮平行移動を行う場合、適切な引数を渡すこと
	void draw(float fScale = 1.f, float fOffsetX = 0.f, float fOffsetY = 0.f);

	/// @brief 描画から除外する全スロット名を登録。二者択一でコールバック関数が優先。 
	void setLeaveOutList(spine::Vector<spine::String>& list) noexcept;
	/// @brief 描画除外判定を行う関数を登録。名称一致より複雑な判定を行う場合に使用。
	void setLeaveOutCallback(LeaveOutCallback pFunc, void* pUserDatum) noexcept;

	/// @brief 全体の境界矩形を算出
	/// @return x座標、y座標、幅、高さの順で格納された矩形
	/// @remark 有効な頂点を有していない場合は無限遠点を格納
	D2D_VECTOR_4F getBoundingBox() noexcept;
	/// @brief 特定スロットの境界矩形を算出
	/// @param found 任意。スロットが存在した場合true、存在しなかった場合false
	/// @return x座標、y座標、幅、高さの順で格納された矩形
	/// @remark スロットが存在したとしても、有効な頂点を有していなかった場合は無限遠点を格納
	D2D_VECTOR_4F getBoundingBoxOfSlot(const char* slotName, size_t nameLength, bool* found = nullptr) noexcept;
private:
	bool m_isAlphaPremultiplied = true;
	bool m_isToForceBlendModeNormal = false;
	bool m_isVisible = true;
	bool m_isPaused = false;
	Physics m_physics = Physics::Update;

	SpineSharedPtr<spine::Atlas> m_atlas;
	SpineSharedPtr<spine::SkeletonData> m_skeletonData;
	/// @brief ユーザ定義データ。ここではCD3D11Renderer2D
	void* m_pRendererObject = nullptr;

	SpineUniquePtr<spine::Skeleton> m_skeleton;
	SpineUniquePtr<spine::AnimationStateData> m_animationStateData;
	SpineUniquePtr<spine::AnimationState> m_animationState;
	spine::SkeletonClipping m_clipper;

	/// @brief 境界矩形を算出するためのバッファ
	spine::Vector<float> m_tempVertices;

	spine::Vector<float> m_worldVertices;
	spine::Vector<d3d11_renderer2d::Vertex2D> m_vertices2D;
	spine::Vector<d3d11_renderer2d::TriangleIndex> m_triangleIndices;

	spine::Vector<spine::String> m_leaveOutList;

	LeaveOutCallback m_pLeaveOutCallback = nullptr;
	void* m_pLeaveOutCallackUserDatum = nullptr;

	bool isToBeLeftOut(const spine::String& slotName) noexcept;
};

#endif // !D3D11_SPINE_DRAWABLE_H_
