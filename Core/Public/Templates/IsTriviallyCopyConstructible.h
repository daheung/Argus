#pragma once

template <typename T>
struct TIsTriviallyCopyConstructible
{
	enum { Value = __is_trivially_copyable(T) };
};
