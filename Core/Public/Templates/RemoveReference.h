#pragma once

template <typename T>
struct TRemoveReference
{
	using Type = T;
};

template <typename T> struct TRemoveReference<T& > { using Type = T; };
template <typename T> struct TRemoveReference<T&&> { using Type = T; };