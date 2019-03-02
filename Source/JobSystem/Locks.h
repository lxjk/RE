#pragma once

#include <atomic>

class SpinLock
{
public:
	__forceinline void LockReadWrite()
	{
		while (lock.test_and_set(std::memory_order_acquire))
			;
	}
	__forceinline void UnlockReadWrite()
	{
		lock.clear(std::memory_order_release);
	}
	__forceinline void LockReadOnly()
	{
		LockReadWrite();
	}
	__forceinline void UnlockReadOnly()
	{
		UnlockReadWrite();
	}
protected:
	std::atomic_flag lock = ATOMIC_FLAG_INIT;
};

// http://joeduffyblog.com/2009/01/29/a-singleword-readerwriter-spin-lock/
class SpinLockReaderWriter
{
public:
	__forceinline void LockReadWrite()
	{
		int state = lock.load(std::memory_order_acquire);
		while (1)
		{
			// if no writer and no reader (all 0 other than waiting bit)
			if (((state & ~MASK_WRITER_WAITING_BIT) == 0))
			{
				// set to writer lock bit
				if(lock.compare_exchange_strong(state, MASK_WRITER_LOCK_BIT, std::memory_order_acq_rel))
					return;
			}
			// otherwise set waiting bit if not set
			if ((state & MASK_WRITER_WAITING_BIT) == 0)
			{
				// we don't care if we failed here
				lock.compare_exchange_strong(state, state | MASK_WRITER_WAITING_BIT, std::memory_order_acq_rel);
			}
			else
			{
				state = lock.load(std::memory_order_acquire);
			}
		}
	}
	__forceinline void UnlockReadWrite()
	{
		// only keep waiting bit in case other writer want to get in
		int prevState = lock.fetch_and(MASK_WRITER_WAITING_BIT, std::memory_order_acq_rel);
		if ((prevState & MASK_READER_BITS) != 0)
		{
			// this happens when we unlock with some readers
			assert(false);
			return;
		}
	}
	__forceinline void LockReadOnly()
	{
		int state = lock.load(std::memory_order_acquire);
		while (1)
		{
			// make sure we don't have writer, and we are not over max reader
			if ((state & MASK_WRITER_BITS) == 0 && (state & MASK_READER_BITS) < MASK_READER_BITS)
			{
				if (lock.compare_exchange_strong(state, state + 1, std::memory_order_acq_rel))
					return;
			}
			else
			{
				state = lock.load(std::memory_order_acquire);
			}
		}
	}
	__forceinline void UnlockReadOnly()
	{
		int prevState = lock.fetch_sub(1, std::memory_order_acq_rel);// make sure we have reader
		if ((prevState & MASK_READER_BITS) == MASK_READER_BITS)
		{
			// this happens when we unlock without any reader
			assert(false);
			return;
		}
	}

protected:
	std::atomic<int> lock = 0;

	const int MASK_WRITER_LOCK_BIT = 0x80000000;
	const int MASK_WRITER_WAITING_BIT = 0x40000000;
	const int MASK_WRITER_BITS = MASK_WRITER_LOCK_BIT | MASK_WRITER_WAITING_BIT;
	const int MASK_READER_BITS = ~MASK_WRITER_BITS;
};

template<class T, class TLock>
class ThreadProtected
{
protected:
	T value;
	TLock lock;

public:
	class ReadWriteScope
	{
	public:
		T& Get() { return valueRef; }

		~ReadWriteScope() { protectedValueRef.ReleaseReadWrite(); }
		ReadWriteScope(ThreadProtected<T, TLock>& protectedValue) :
			protectedValueRef(protectedValue),
			valueRef(protectedValue.AcquireReadWrite())
		{}

	private:
		ReadWriteScope(const ThreadProtected<T, TLock>& other);
		ReadWriteScope(const ThreadProtected<T, TLock>&& other);

		ThreadProtected<T, TLock>& protectedValueRef;
		T& valueRef;

		friend ThreadProtected<T, TLock>;
	};

	class ReadOnlyScope
	{
	public:
		T& Get() { return valueRef; }

		~ReadOnlyScope() { protectedValueRef.ReleaseReadOnly(); }
		ReadOnlyScope(ThreadProtected<T, TLock>& protectedValue) :
			protectedValueRef(protectedValue),
			valueRef(protectedValue.AcquireReadOnly())
		{}

	private:
		ReadOnlyScope(const ThreadProtected<T, TLock>& other);
		ReadOnlyScope(const ThreadProtected<T, TLock>&& other);

		ThreadProtected<T, TLock>& protectedValueRef;
		T& valueRef;

		friend ThreadProtected<T, TLock>;
	};

	typedef T value_type;
	typedef TLock lock_type;

	__forceinline T& AcquireReadWrite()
	{
		lock.LockReadWrite();
		return value;
	}

	__forceinline void ReleaseReadWrite()
	{
		lock.UnlockReadWrite();
	}

	__forceinline const T& AcquireReadOnly()
	{
		lock.LockReadOnly();
		return value;
	}

	__forceinline void ReleaseReadOnly()
	{
		lock.UnlockReadOnly();
	}
};

#define SCOPED_READ_WRITE_REF(name, ref) decltype(name)::ReadWriteScope __##name_scope(name);\
auto& ref = __##name_scope.Get();

#define SCOPED_READ_ONLY_REF(name, ref) decltype(name)::ReadOnlyScope __##name_scope(name);\
auto& ref = __##name_scope.Get();