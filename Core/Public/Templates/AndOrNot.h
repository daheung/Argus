#pragma once

template <typename T>
struct TNot
{
	static constexpr bool Value = !T::Value;
};

template <bool Predicate, typename... Args>
struct TAndValue;

template <typename T, typename... Args>
struct TAnd
{
	static constexpr bool Value = TAndValue<T::Value, Args...>::Value;
};

template <typename T, typename... Args>
struct TAndValue<true, T, Args...> 
{
	static constexpr bool Value = TAndValue<T::Value, Args...>::Value;
};

template <typename T, typename... Args>
struct TAndValue<false, T, Args...>
{
	static constexpr bool Value = false;
};

template<> struct TAndValue<true > { static constexpr bool Value = true; };
template<> struct TAndValue<false> { static constexpr bool Value = false; };

template <bool Predicate, typename... Args>
struct TOrValue;

template <typename T, typename... Args>
struct TOr
{
	static constexpr bool Value = TOrValue<T::Value, Args...>::Value;
};

template <typename T, typename... Args>
struct TOrValue<true, T, Args...>
{
	static constexpr bool Value = true;
};

template <typename T, typename... Args>
struct TOrValue<false, T, Args...>
{
	static constexpr bool Value = TOrValue<T::Value, Args...>::Value;
};

template<> struct TOrValue<true > { static constexpr bool Value = true; };
template<> struct TOrValue<false> { static constexpr bool Value = false; };