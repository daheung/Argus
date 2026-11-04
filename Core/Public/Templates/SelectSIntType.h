#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/MakeUnsigned.h"

template <int Size>
struct TSelectSIntType
{
};

template <> struct TSelectSIntType<1> { using Type = int8 ; };
template <> struct TSelectSIntType<2> { using Type = int16; };
template <> struct TSelectSIntType<4> { using Type = int32; };
template <> struct TSelectSIntType<8> { using Type = int64; };

template <int Size>
struct TSelectUIntType
{
	using Type = typename TMakeUnsigned<typename TSelectSIntType<Size>::Type>::Type;
};

template <int Size>
struct TSelectSLongType
{
};

template <> struct TSelectSLongType<4> { using Type = long; };
template <> struct TSelectSLongType<8> { using Type = long long; };

template <int Size>
struct TSelectULongType
{
	using Type = typename TMakeUnsigned<typename TSelectSLongType<Size>::Type>::Type;
};