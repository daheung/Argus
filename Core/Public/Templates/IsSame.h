#pragma once

template <typename T, typename U>
struct TIsSame
{
	static constexpr bool Value = false;
};

template <typename T>
struct TIsSame<T, T>
{
	static constexpr bool Value = true;
};