#ifndef SPINE_SHARED_PTR_H_
#define SPINE_SHARED_PTR_H_

/*
* Intended to be used to manage sharing of
* (1) spine::Atlas and spine::SkeletonData in C++ generic runtime and
* (2) spAtlas and spSkeletonData in C generic runtime
* in the environment where STL <memory> cannot be used.
*
* Mind that atomic operation is not performed for reference counter.
*
*/

#include <new>

template <typename T>
class SpineSharedPtr
{
public:
	SpineSharedPtr() noexcept = default;

	SpineSharedPtr(T* ptr)
	{
		if (ptr != nullptr)
		{
			void* pRefBlockMemory = ::operator new(sizeof(DefaultRefCountBlock), std::nothrow);
			if (pRefBlockMemory != nullptr)
			{
				m_ptr = ptr;
				m_pRefCountBlock = new (pRefBlockMemory) DefaultRefCountBlock();
			}
		}
	}

	template <typename D>
	SpineSharedPtr(T* ptr, D customDeleter)
	{
		if (ptr != nullptr)
		{
			void* pRefBlockMemory = ::operator new(sizeof(CustomRefCountBlock<D>), std::nothrow);
			if (pRefBlockMemory != nullptr)
			{
				m_ptr = ptr;
				m_pRefCountBlock = new (pRefBlockMemory) CustomRefCountBlock<D>(static_cast<D&&>(customDeleter));
			}
		}
	}

	template <typename... Args>
	static SpineSharedPtr MakeShared(Args&&... args)
	{
		SpineSharedPtr pThis;

		void* pWholeMemory = ::operator new(sizeof(MakeSharedRefCountBlock), std::nothrow);
		if (pWholeMemory == nullptr)
		{
			return pThis;
		}

		MakeSharedRefCountBlock* pRefCountBlock = new (pWholeMemory) MakeSharedRefCountBlock(static_cast<Args&&>(args)...);

		pThis.m_ptr = pRefCountBlock->ptrMemory();
		pThis.m_pRefCountBlock = pRefCountBlock;

		return pThis;
	}

	SpineSharedPtr(const SpineSharedPtr& other)
		: m_ptr(other.m_ptr), m_pRefCountBlock(other.m_pRefCountBlock)
	{
		if (m_pRefCountBlock != nullptr)
		{
			m_pRefCountBlock->incRef();
		}
	}

	SpineSharedPtr(SpineSharedPtr&& other) noexcept
		: m_ptr(other.m_ptr), m_pRefCountBlock(other.m_pRefCountBlock)
	{
		other.m_ptr = nullptr;
		other.m_pRefCountBlock = nullptr;
	}

	~SpineSharedPtr()
	{
		release();
	}

	SpineSharedPtr& operator=(const SpineSharedPtr& other) noexcept
	{
		if (this != &other)
		{
			release();

			m_ptr = other.m_ptr;
			m_pRefCountBlock = other.m_pRefCountBlock;

			if (m_pRefCountBlock != nullptr)
			{
				m_pRefCountBlock->incRef();
			}
		}

		return *this;
	}

	SpineSharedPtr& operator=(SpineSharedPtr&& other) noexcept
	{
		if (this != &other)
		{
			release();

			m_ptr = other.m_ptr;
			m_pRefCountBlock = other.m_pRefCountBlock;

			other.m_ptr = nullptr;
			other.m_pRefCountBlock = nullptr;
		}

		return *this;
	}

	T* get() const noexcept { return m_ptr; }
	int count() const noexcept
	{
		return m_pRefCountBlock == nullptr ? 0 : m_pRefCountBlock->count();
	}
	bool hasRefCountBlock() const noexcept
	{
		return m_pRefCountBlock != nullptr;
	}

	T& operator*() const noexcept { return *m_ptr; }
	T* operator->() const noexcept { return m_ptr; }

	explicit operator bool() const noexcept { return m_ptr != nullptr; }
	bool operator==(decltype(nullptr)) const noexcept { return m_ptr == nullptr; }
	bool operator!=(decltype(nullptr)) const noexcept { return m_ptr != nullptr; }
private:
	class RefCountBlock
	{
	public:
		RefCountBlock() = default;
		virtual ~RefCountBlock() = default;

		void incRef() noexcept
		{
			m_refCount++;
		}

		bool decRef() noexcept
		{
			m_refCount--;

			return m_refCount == 0;
		}

		int count() const noexcept
		{
			return m_refCount;
		}

		virtual void destroy(T* ptr) noexcept = 0;

	private:
		int m_refCount = 1;
	};

	class DefaultRefCountBlock : public RefCountBlock
	{
	public:
		void destroy(T* ptr) noexcept override
		{
			delete ptr;

			DefaultRefCountBlock::~DefaultRefCountBlock();
			::operator delete(this);
		}
	};

	template <typename D>
	class CustomRefCountBlock : public RefCountBlock
	{
	public:
		explicit CustomRefCountBlock(D deleter)
			: m_deleter(static_cast<D&&>(deleter))
		{

		}

		void destroy(T* ptr) noexcept override
		{
			m_deleter(ptr);

			CustomRefCountBlock::~CustomRefCountBlock();
			::operator delete(this);
		}

	private:
		D m_deleter;
	};

	class MakeSharedRefCountBlock : public RefCountBlock
	{
	public:
		template <typename... Args>
		explicit MakeSharedRefCountBlock(Args&&... args)
		{
			new (m_ptrMemory) T(static_cast<Args&&>(args)...);
		}

		T* ptrMemory() noexcept
		{
			return reinterpret_cast<T*>(m_ptrMemory);
		}

		void destroy(T* /*ptr*/) noexcept override
		{
			ptrMemory()->~T();

			MakeSharedRefCountBlock::~MakeSharedRefCountBlock();
			::operator delete(this);
		}

	private:
		alignas(T) unsigned char m_ptrMemory[sizeof(T)];
	};

	T* m_ptr = nullptr;
	RefCountBlock* m_pRefCountBlock = nullptr;

	void release()
	{
		if (m_pRefCountBlock != nullptr)
		{
			if (m_pRefCountBlock->decRef())
			{
				m_pRefCountBlock->destroy(m_ptr);
			}
		}

		m_ptr = nullptr;
		m_pRefCountBlock = nullptr;
	}
};

#endif // !SPINE_SHARED_PTR_H_
