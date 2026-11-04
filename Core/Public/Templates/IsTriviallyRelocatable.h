#pragma once

#include "Core/Public/Templates/AndOrNot.h"
#include "Core/Public/Templates/IsTriviallyDestructible.h"
#include "Core/Public/Templates/IsTriviallyMoveConstructible.h"

/**
 * Whether T can be relocated by moving its bytes to new storage without
 * invoking T's move constructor or the source object's destructor.
 *
 * TIsBitwiseConstructible<Dest, Source> asks whether copying Source's bytes
 * creates another valid object while Source remains alive. Relocation moves
 * one object's state to new storage and abandons the source object, so it
 * does not require two independently owned objects. A type may therefore be
 * bitwise relocatable even when bitwise construction would duplicate ownership.
 *
 * Unlike TIsTriviallyMoveConstructible and TIsTriviallyDestructible, this
 * trait describes whether bitwise relocation is valid for a particular type,
 * not whether its special member functions are implemented trivially.
 * Their values provide the default, but cannot prove that address-dependent
 * invariants survive relocation. A type with non-trivial move construction
 * or destruction may specialize this trait to true when its ownership and
 * address invariants remain valid after bitwise relocation.
 */
template <typename T>
struct TIsTriviallyRelocatable
{
	static constexpr bool Value = TAnd<TIsTriviallyMoveConstructible<T>, TIsTriviallyDestructible<T>>::Value;
};
