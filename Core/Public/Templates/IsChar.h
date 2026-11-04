#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/AndOrNot.h"
#include "Core/Public/Templates/IsSame.h"

template <typename T>
struct TIsChar
{
	static constexpr bool Value = TOr<
		TIsSame<T, WIDECHAR >, 
		TIsSame<T, UTF16CHAR>
	>::Value;
};

template <> struct TIsChar<ANSICHAR> { enum { Value = true }; };
//template <> struct TIsChar<WIDECHAR> { enum { Value = true }; };

template <> struct TIsChar<UTF8CHAR > { enum { Value = true }; };
//template <> struct TIsChar<UTF16CHAR> { enum { Value = true }; };
template <> struct TIsChar<UTF32CHAR> { enum { Value = true }; };

template <typename T> struct TIsChar<const			T> { enum { Value = TIsChar<T>::Value }; };
template <typename T> struct TIsChar<	   volatile T> { enum { Value = TIsChar<T>::Value }; };
template <typename T> struct TIsChar<const volatile T> { enum { Value = TIsChar<T>::Value }; };