#pragma once

#include "Core/Public/Templates/TypeTraits.h"

namespace Core::Templates
{
	template <typename T>
	struct TUseBitwiseSwap
	{
		enum
		{
			Value = TOr<TIsEnum<T>, TIsPointer<T>, TIsArithmetic<T>>::Value
		};
	};
}

template <typename T>
constexpr T&& Forward(typename TRemoveReference<T>::Type& Obj) noexcept
{
	return static_cast<T&&>(Obj);
}

template <typename T>
constexpr T&& Forward(typename TRemoveReference<T>::Type&& Obj) noexcept
{
	return static_cast<T&&>(Obj);
}

template <typename FuncType, typename ...ArgsType>
FORCEINLINE auto Invoke(FuncType&& Func, ArgsType&&... Args)
-> decltype(Forward<FuncType>(Func)(Forward<ArgsType>(Args)...))
{
	return Forward<FuncType>(Func)(Forward<ArgsType>(Args)...);
}

template <typename FuncType, typename ObjectType, typename... ArgsType>
FORCEINLINE auto Invoke(FuncType&& Func, ObjectType&& Object, ArgsType&&... Args)
-> decltype(Object.*Forward<FuncType>(Func)(Forward<ArgsType>(Args)...))
{
	return Object.*Forward<FuncType>(Func)(Forward<ArgsType>(Args)...);
}

template <typename T>
FORCEINLINE typename TRemoveReference<T>::Type&& MoveTemp(T&& Object)
{
	using ToMoveType = typename TRemoveReference<T>::Type;

	static_assert(TIsLValueReference<T>::Value, "MoveTemp called on an rvalue");
	static_assert(TIsSame<T&, const T&>::Value, "MoveTemp called on a const object");

	return static_cast<ToMoveType&&>(Object);
}

template <typename T>
FORCEINLINE typename TRemoveReference<T>::Type&& MoveTempIfPossible(T&& Object)
{
	using ToMoveType = typename TRemoveReference<T>::Type;
	return static_cast<ToMoveType&&>(Object);
}

template <typename T>
FORCEINLINE void Swap(T& A, T& B)
{
	if constexpr (Core::Templates::TUseBitwiseSwap<T>::Value)
	{
		T Temp = MoveTemp(A);
		A = MoveTemp(B);
		B = MoveTemp(Temp);
	}
	else
	{
		T Temp = MoveTemp(A);
		A = MoveTemp(B);
		B = MoveTemp(Temp);
	}
}

template <typename T>
constexpr T&& DeclVal() noexcept;

template <typename T>
FORCEINLINE T ImplicitConv(typename TIdentity<T>::Type Object)
{
	return Object;
}