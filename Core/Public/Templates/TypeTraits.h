#pragma once

/*
 * Kernel-mode compatibility:
 *
 * This project intentionally avoids using the C++ Standard Library
 * to prevent potential dependencies on CRT and exception handling
 * facilities that may be unavailable in kernel-mode environments.
 *
 * Although most type traits are evaluated at compile time, standard
 * library usage can introduce unwanted dependencies depending on
 * the compiler and build configuration.
 *
 * To maintain kernel compatibility and minimize dependencies,
 * only the required type traits are implemented locally, as needed.
 */

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/Decay.h"
#include "Core/Public/Templates/IsSame.h"
#include "Core/Public/Templates/IsChar.h"
#include "Core/Public/Templates/IsEnum.h"
#include "Core/Public/Templates/IsEmpty.h"
#include "Core/Public/Templates/IsArray.h"
#include "Core/Public/Templates/IsSigned.h"
#include "Core/Public/Templates/IsPointer.h"
#include "Core/Public/Templates/IsIdentity.h"
#include "Core/Public/Templates/IsReference.h"
#include "Core/Public/Templates/IsArithmetic.h"
#include "Core/Public/Templates/IsContiguousContainer.h"
#include "Core/Public/Templates/IsValidVariadicFunctionArg.h"
#include "Core/Public/Templates/IsArrayOrRefOfTypeByPredicate.h"
#include "Core/Public/Templates/IsCharEncodingSimplyConvertibleTo.h"
#include "Core/Public/Templates/AndOrNot.h"
#include "Core/Public/Templates/Element.h"
#include "Core/Public/Templates/EnableIf.h"
#include "Core/Public/Templates/MakeSigned.h"
#include "Core/Public/Templates/MakeUnsigned.h"
#include "Core/Public/Templates/SelectSIntType.h"
#include "Core/Public/Templates/IsMoveConstructible.h"
#include "Core/Public/Templates/IsTriviallyCopyConstructible.h"
#include "Core/Public/Templates/IsTriviallyMoveConstructible.h"
#include "Core/Public/Templates/IsTriviallyDestructible.h"
#include "Core/Public/Templates/IsTriviallyRelocatable.h"
#include "Core/Public/Templates/RemoveConstVolatile.h"
#include "Core/Public/Templates/RemoveReference.h"
#include "Core/Public/Templates/RemovePointer.h"
#include "Core/Public/Templates/Conditional.h"

#define TEMPLATE_REQUIRES(...) \
	typename = typename TEnableIf<(__VA_ARGS__), void>::Type

template <typename DerivedType, typename BaseType>
struct TIsDerivedFrom
{
private:
	typedef char No [1];
	typedef char Yes[2];

	static constexpr FORCEINLINE Yes& Test(BaseType*) noexcept;
	static constexpr FORCEINLINE Yes& Test(const BaseType*) noexcept;
	static constexpr FORCEINLINE No&  Test(...) noexcept;

	static constexpr FORCEINLINE DerivedType* DerivedTypePtr() noexcept { return nullptr; }

public:
	static constexpr bool Value = sizeof(Test(DerivedTypePtr())) == sizeof(Yes);

	static constexpr bool IsDerived = Value;
};

template <typename T>
struct TRemoveCVReference
{
	using Type = typename TRemoveConstVolatile<typename TRemoveReference<T>::Type>::Type;
};

template <typename T, typename Arg>
struct TIsBitwiseConstructible
{
	// Reference types must not be treated as bitwise-constructible.
	// Doing so could allow raw memory copying of the referenced object,
	// bypassing non-trivial copy/move constructors and potentially
	// corrupting resource ownership or violating object invariants.
	static_assert(
		!TIsReferenceType<T>::Value && !TIsReferenceType<Arg>::Value,
		"TIsBitwiseConstructible does not support reference types"
	);

	enum { Value = false };
};

template <typename T>
struct TIsBitwiseConstructible<T, T>
{
	enum { Value = TIsTriviallyCopyConstructible<T>::Value };
};

template <typename T>
struct TIsBitwiseConstructible<T*, T*>
{
	enum { Value = true };
};

template <typename T, typename U>
struct TIsBitwiseConstructible<const T, U> : TIsBitwiseConstructible<T, U>
{

};

template <typename T>
struct TIsBitwiseConstructible<const T*, T*>
{
	enum { Value = true };
};

//template <typename T>
//struct TIsBitwiseConstructible<const T&, T&>
//{
//	enum { Value = true };
//};
//
//template <typename T>
//struct TIsBitwiseConstructible<const T&&, T&&>
//{
//	enum { Value = true };
//};

template <> struct TIsBitwiseConstructible< uint8,   int8> { enum { Value = true }; };
template <> struct TIsBitwiseConstructible<  int8,  uint8> { enum { Value = true }; };
template <> struct TIsBitwiseConstructible<uint16,  int16> { enum { Value = true }; };
template <> struct TIsBitwiseConstructible< int16, uint16> { enum { Value = true }; };
template <> struct TIsBitwiseConstructible<uint32,  int32> { enum { Value = true }; };
template <> struct TIsBitwiseConstructible< int32, uint32> { enum { Value = true }; };
template <> struct TIsBitwiseConstructible<uint64,  int64> { enum { Value = true }; };
template <> struct TIsBitwiseConstructible< int64, uint64> { enum { Value = true }; };