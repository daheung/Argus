#pragma once

template <bool Predicate, typename Result = void>
struct TEnableIf
{
};

template <typename Result>
struct TEnableIf<true, Result>
{
	using Type = Result;
	using type = Result;
};

template <typename Result>
struct TEnableIf<false, Result>
{
};
