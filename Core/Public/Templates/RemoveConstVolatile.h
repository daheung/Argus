#pragma once

template <typename T>
struct TRemoveConstVolatile
{
	using Type = T;
};

template <typename T> struct TRemoveConstVolatile<const			 T> { using Type = T; };
template <typename T> struct TRemoveConstVolatile<		volatile T> { using Type = T; };
template <typename T> struct TRemoveConstVolatile<const volatile T> { using Type = T; };