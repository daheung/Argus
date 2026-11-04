#pragma once

#include "Core/Public/Misc/Types.h"

template <typename T>
struct TIsArray
{
	enum { Value = false };
};

template <typename T>			struct TIsArray<T[]> { enum { Value = true }; };
template <typename T, size_t N> struct TIsArray<T[N]> { enum { Value = true }; };


template <typename T>
struct TIsBoundedArray
{
	enum { Value = false };
};

template <typename T, size_t N> struct TIsBoundedArray<T[N]> { enum { Value = true }; };


template <typename T>
struct TIsUnboundedArray
{
	enum { Value = false };
};

template <typename T> struct TIsUnboundedArray<T[]> { enum { Value = true }; };