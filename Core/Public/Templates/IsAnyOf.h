#pragma once

#include "Core/Public/Templates/AndOrNot.h"
#include "Core/Public/Templates/IsSame.h"

template <typename T, typename Type, typename... Types>
struct TIsAnyOfValue;

template <typename T, typename... Types>
struct TIsAnyOf 
{
	static constexpr bool Value = TIsAnyOfValue<T, Types...>::Value;
};

template <typename T, typename Type, typename... Types>
struct TIsAnyOfValue
{
	static constexpr bool Value = TOr<TIsSame<T, Type>, TIsAnyOfValue<T, Types...>>::Value;
};

template <typename T, typename Type>
struct TIsAnyOfValue<T, Type>
{
	static constexpr bool Value = TIsSame<T, Type>::Value;
};