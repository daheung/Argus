#pragma once

template <typename T>
struct TIsTriviallyDestructible
{
	enum { Value = __is_trivially_destructible(T) };
};