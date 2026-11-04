#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Templates/TypeTraits.h"



namespace Atomic
{
	enum class EMemoryOrder : uint8
	{
		Relaxed,
		Acquire,
		Release,
		AcqRel,
		SeqCst
	};
}

#if defined(PLATFORM_WDM) || defined(PLATFORM_KMDF)
#include "Kernel/Public/Misc/KernelDefines.h"

// No C++ STL dependency. 32/64-bit atomic counters using WDK Interlocked APIs.
// RefCounter must be a 32- or 64-bit integral type. Counts must not overflow.
template <unsigned N>
struct TKernelInterlockedOps;

template <>
struct TKernelInterlockedOps<4>
{
	using NativeType = typename TSelectSLongType<4>::Type;

	static FORCEINLINE NativeType Load(volatile NativeType* P) noexcept
	{
		return _InterlockedCompareExchange(P, 0, 0);
	}

	static FORCEINLINE NativeType FetchAdd(volatile NativeType* P, NativeType N) noexcept
	{
		return _InterlockedExchangeAdd(P, N);
	}

	static FORCEINLINE NativeType Exchange(volatile NativeType* P, NativeType N) noexcept
	{
		return _InterlockedExchange(P, N);
	}

	static FORCEINLINE NativeType CompareExchange(volatile NativeType* P, NativeType Desired, NativeType Expected) noexcept
	{
		return _InterlockedCompareExchange(P, Desired, Expected);
	}
};

template <>
struct TKernelInterlockedOps<8>
{
	using NativeType = TSelectSLongType<8>::Type;

	static FORCEINLINE NativeType Load(volatile NativeType* P) noexcept
	{
		return _InterlockedCompareExchange64(P, 0, 0);
	}

	static FORCEINLINE NativeType FetchAdd(volatile NativeType* P, NativeType N) noexcept
	{
		return _InterlockedExchangeAdd64(P, N);
	}

	static FORCEINLINE NativeType Exchange(volatile NativeType* P, NativeType N) noexcept
	{
		return _InterlockedExchange64(P, N);
	}

	static FORCEINLINE NativeType CompareExchange(volatile NativeType* P, NativeType Desired, NativeType Expected) noexcept
	{
		return _InterlockedCompareExchange64(P, Desired, Expected);
	}
};

template <typename T>
class TKernelAtomicCounter
{
	static_assert(sizeof(T) == 4 || sizeof(T) == 8, "RefCounter must be a 32-bit or 64-bit integer");

	using Ops = TKernelInterlockedOps<sizeof(T)>;
	using NativeType = typename Ops::NativeType;

	alignas(sizeof(NativeType)) mutable volatile NativeType Value;

public:
	explicit TKernelAtomicCounter(T Initial = 0) noexcept
		: Value(static_cast<NativeType>(Initial))
	{
	}

	TKernelAtomicCounter(const TKernelAtomicCounter&) = delete;
	TKernelAtomicCounter& operator=(const TKernelAtomicCounter&) = delete;

	FORCEINLINE T load(Atomic::EMemoryOrder MemoryOrder) const noexcept
	{
		UNREFERENCED_PARAMETER(MemoryOrder);
		return static_cast<T>(Ops::Load(&Value));
	}

	FORCEINLINE void store(T NewValue, Atomic::EMemoryOrder MemoryOrder) noexcept
	{
		UNREFERENCED_PARAMETER(MemoryOrder);
		Ops::Exchange(&Value, static_cast<NativeType>(NewValue));
	}

	FORCEINLINE T fetch_add(T Amount, Atomic::EMemoryOrder MemoryOrder) noexcept
	{
		UNREFERENCED_PARAMETER(MemoryOrder);
		return static_cast<T>(Ops::FetchAdd(&Value, static_cast<NativeType>(Amount)));
	}

	FORCEINLINE T fetch_sub(T Amount, Atomic::EMemoryOrder MemoryOrder) noexcept
	{
		UNREFERENCED_PARAMETER(MemoryOrder);
		return static_cast<T>(Ops::FetchAdd(&Value, -static_cast<NativeType>(Amount)));
	}

	// A strong CAS also satisfies compare_exchange_weak's contract.
	FORCEINLINE bool compare_exchange_weak(T& Expected, T Desired, Atomic::EMemoryOrder MemoryOrder) noexcept
	{
		UNREFERENCED_PARAMETER(MemoryOrder);
		const NativeType NativeExpected = static_cast<NativeType>(Expected);
		const NativeType Original = Ops::CompareExchange(&Value, static_cast<NativeType>(Desired), NativeExpected);

		if (Original != NativeExpected)
		{
			Expected = static_cast<T>(Original);
			return false;
		}

		return true;
	}

	FORCEINLINE bool compare_exchange_strong(T& Expected, T Desired, Atomic::EMemoryOrder MemoryOrder) noexcept
	{
		return compare_exchange_weak(Expected, Desired, MemoryOrder);
	}
};

#endif
