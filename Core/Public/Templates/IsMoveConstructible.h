#pragma once

template <typename T>
struct TIsMoveConstructible
{
	static constexpr bool Value = __is_constructible(T, T&&);
};

template <typename T, typename U>
struct TIsMoveConstructibleFrom
{
	static constexpr bool Value = __is_constructible(T, U&&);
};

template <typename T>
struct TIsMoveConstructibleFrom<T, T> :
	public TIsMoveConstructible<T>
{
};