#pragma once

#include <cstdint>	/* for uintptr_t */
#include <cstdlib>
#include <limits>
#include <type_traits>
#include <xutility>

// we set _SECURE_SCL=0;_HAS_ITERATOR_DEBUGGING=0; in preprocessor in Debug/X64 for performance

inline __declspec(allocator) void* _Aligned_Allocate(size_t _Count, size_t _Sz, size_t _Alignment)
{
	void *_Ptr = 0;

	if (_Count == 0)
		return (_Ptr);

	// check overflow of multiply
	if ((size_t)(-1) / _Sz < _Count)
		std::_Xbad_alloc();	// report no memory
	const size_t _User_size = _Count * _Sz;

	{	// allocate normal block
		_Ptr = _aligned_malloc(_User_size, _Alignment);
		_STL_VERIFY(_Ptr != 0, "invalid argument");	// validate even in release since we're doing p[-1]
	}
	return (_Ptr);
}

inline void _Aligned_Deallocate(void * _Ptr)
{
	_aligned_free(_Ptr);
}

template<class T>
class REAllocatorBase
{
public:
	static_assert(!std::is_const<T>::value,
		"The C++ Standard forbids containers of const elements "
		"because REAllocator<const T> is ill-formed.");

	typedef void _Not_user_specialized;

	typedef T value_type;

	typedef T *pointer;
	typedef const T *const_pointer;

	typedef T& reference;
	typedef const T& const_reference;

	typedef size_t size_type;
	typedef ptrdiff_t difference_type;

	typedef std::true_type propagate_on_container_move_assignment;
	typedef std::true_type is_always_equal;
	
	pointer address(reference _Val) const noexcept
	{	// return address of mutable _Val
		return (_STD addressof(_Val));
	}

	const_pointer address(const_reference _Val) const noexcept
	{	// return address of nonmutable _Val
		return (_STD addressof(_Val));
	}

	template<class _Objty,
		class... _Types>
		void construct(_Objty *_Ptr, _Types&&... _Args)
	{	// construct _Objty(_Types...) at _Ptr
		::new ((void *)_Ptr) _Objty(_STD forward<_Types>(_Args)...);
	}

	template<class _Uty>
	void destroy(_Uty *_Ptr)
	{	// destroy object at _Ptr
		_Ptr->~_Uty();
	}

	size_t max_size() const noexcept
	{	// estimate maximum array size
		return ((size_t)(-1) / sizeof(T));
	}
};

template<class T, int Alignment>
class REAllocator : public REAllocatorBase<T>
{
public:
	template<class _Other>
	struct rebind
	{	// convert this type to REAllocator<_Other, Alignment>
		typedef REAllocator<_Other, Alignment> other;
	};

	REAllocator() noexcept
	{	// construct default REAllocator (do nothing)
	}

	REAllocator(const REAllocator<T, Alignment>&) noexcept
	{	// construct by copying (do nothing)
	}

	template<class _Other>
	REAllocator(const REAllocator<_Other, Alignment>&) noexcept
	{	// construct from a related REAllocator (do nothing)
	}

	template<class _Other>
	REAllocator<T, Alignment>& operator=(const REAllocator<_Other, Alignment>&)
	{	// assign from a related REAllocator (do nothing)
		return (*this);
	}

	void deallocate(pointer _Ptr, size_type _Count)
	{	// deallocate object at _Ptr
		_Aligned_Deallocate(_Ptr);
	}

	__declspec(allocator) pointer allocate(size_type _Count, const void * hint = 0)
	{	// allocate array of _Count elements, ignore hint
		return (static_cast<pointer>(_Aligned_Allocate(_Count, sizeof(T), Alignment)));
	}
};

template<class _Ty,	class _Other, int Alignment> inline
	bool operator==(const REAllocator<_Ty, Alignment>&,
		const REAllocator<_Other, Alignment>&) noexcept
{	// test for allocator equality
	return (true);
}

template<class _Ty,	class _Other, int Alignment> inline
	bool operator!=(const REAllocator<_Ty, Alignment>& _Left,
		const REAllocator<_Other, Alignment>& _Right) noexcept
{	// test for allocator inequality
	return (false);
}

template<class T>
class REAllocator<T, 0> : public REAllocatorBase<T>
{	// generic allocator for objects of class T
public:
	template<class _Other>
	struct rebind
	{	// convert this type to REAllocator<_Other, 0>
		typedef REAllocator<_Other, 0> other;
	};

	REAllocator() noexcept
	{	// construct default REAllocator (do nothing)
	}

	REAllocator(const REAllocator<T, 0>&) noexcept
	{	// construct by copying (do nothing)
	}

	template<class _Other>
	REAllocator(const REAllocator<_Other, 0>&) noexcept
	{	// construct from a related REAllocator (do nothing)
	}

	template<class _Other>
	REAllocator<T, 0>& operator=(const REAllocator<_Other, 0>&)
	{	// assign from a related REAllocator (do nothing)
		return (*this);
	}

	void deallocate(pointer _Ptr, size_type _Count)
	{	// deallocate object at _Ptr
		// std::_Deallocate(_Ptr, _Count * sizeof(T));
		std::_Deallocate<std::_New_alignof<value_type>>(_Ptr, sizeof(value_type) * _Count);
	}

	__declspec(allocator) pointer allocate(size_type _Count, const void * hint = 0)
	{	// allocate array of _Count elements, ignore hint
		//return (static_cast<pointer>(std::_Allocate(_Count * sizeof(T))));
		return (static_cast<pointer>(std::_Allocate<std::_New_alignof<value_type>>(std::_Get_size_of_n<sizeof(value_type)>(_Count))));
	}
};

template<class _Ty, class _Other> inline
bool operator==(const REAllocator<_Ty, 0>&,
	const REAllocator<_Other, 0>&) noexcept
{	// test for allocator equality
	return (true);
}

template<class _Ty, class _Other> inline
bool operator!=(const REAllocator<_Ty, 0>& _Left,
	const REAllocator<_Other, 0>& _Right) noexcept
{	// test for allocator inequality
	return (false);
}