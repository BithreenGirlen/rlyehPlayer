#ifndef SPINE_UNIQUE_PTR_H
#define SPINE_UNIQUE_PTR_H

/*
* Intended to be used to manage drawable which has been duplicated by sharing
* (1) spine::Atlas and spine::SkeletonData in C++ generic runtime, and
* (2) spAtlas and spSkeletonData in C generic runtime
* in the environment where STL <memory> cannot be used.
*
* Custom deleter is not suported.
*
*/

#include <new>

template <typename T>
class SpineUniquePtr
{
public:
	SpineUniquePtr() noexcept = default;

	SpineUniquePtr(T* ptr) noexcept
		: m_ptr(ptr)
	{

	}

	SpineUniquePtr(SpineUniquePtr&& other) noexcept
		: m_ptr(other.m_ptr)
	{
		other.m_ptr = nullptr;
	}

	~SpineUniquePtr()
	{
		reset();
	}

	template <typename... Args>
	static SpineUniquePtr MakeUnique(Args&&... args)
	{
		/* Spine class inherets spine::Object, which does not allow (std::nothrow) new. */
		return SpineUniquePtr<T>(new T(static_cast<Args&&>(args)...));
	}

	T* release() noexcept
	{
		T* ptr = m_ptr;
		m_ptr = nullptr;

		return ptr;
	}

	void reset(T* ptr = nullptr)
	{
		if (m_ptr != nullptr)
		{
			delete m_ptr;
		}

		m_ptr = ptr;
	}

	SpineUniquePtr& operator=(SpineUniquePtr&& other) noexcept
	{
		if (this != &other)
		{
			reset();

			m_ptr = other.m_ptr;
			other.m_ptr = nullptr;
		}

		return *this;
	}

	T* get() const noexcept { return m_ptr; }
	T& operator*() const noexcept { return *m_ptr; }
	T* operator->() const noexcept { return m_ptr; }

	explicit operator bool() const noexcept { return m_ptr != nullptr; }
	bool operator==(decltype(nullptr)) const noexcept { return m_ptr == nullptr; }
	bool operator!=(decltype(nullptr)) const noexcept { return m_ptr != nullptr; }

	SpineUniquePtr(const SpineUniquePtr&) = delete;
	SpineUniquePtr& operator=(const SpineUniquePtr&) = delete;
private:
	T* m_ptr = nullptr;
};

#endif // !SPINE_UNIQUE_PTR_H
