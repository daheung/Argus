#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/Void.h"

namespace Core::Private::TypeTraits::ElementType
{
	template <typename T, typename = void>
	struct TImpl
	{
	};

	template <typename T>
	struct TImpl<T, typename TVoid<typename T::ElementType>::Type>
	{
		using Type = typename T::ElementType;
	};
}

template <typename T>
struct TElementType :
	public Core::Private::TypeTraits::ElementType::TImpl<T>
{
};

template <typename T> struct TElementType<				T&> : TElementType<T> {};
template <typename T> struct TElementType<			   T&&> : TElementType<T> {};
template <typename T> struct TElementType<const			 T> : TElementType<T> {};
template <typename T> struct TElementType<		volatile T> : TElementType<T> {};
template <typename T> struct TElementType<const volatile T> : TElementType<T> {};

template <typename T, size_t N> struct TElementType<			   T[N]> { using Type = T; };
template <typename T, size_t N> struct TElementType<const		   T[N]> { using Type = T; };
template <typename T, size_t N> struct TElementType<	  volatile T[N]> { using Type = T; };
template <typename T, size_t N> struct TElementType<const volatile T[N]> { using Type = T; };