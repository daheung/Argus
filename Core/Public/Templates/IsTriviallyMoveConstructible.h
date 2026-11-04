#pragma once

template <typename T>
struct TIsTriviallyMoveConstructible
{
	enum { Value = __is_trivially_constructible(T, T&&) };
};
