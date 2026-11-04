#pragma once

template <typename T>
struct TIsReferenceType
{
	enum { Value = false };
};

template <typename T> struct TIsReferenceType<T& > { enum { Value = true }; };
template <typename T> struct TIsReferenceType<T&&> { enum { Value = true }; };


template <typename T>
struct TIsLValueReference
{
	enum { Value = false };
};

template <typename T> struct TIsLValueReference<T& > { enum { Value = true }; };


template <typename T>
struct TIsRValueReference
{
	enum { Value = false };
};

template <typename T> struct TIsRValueReference<T&&> { enum { Value = true }; };