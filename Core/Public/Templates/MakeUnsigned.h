#pragma once

#include "Core/Public/Misc/Types.h"

template <typename T>
struct TMakeUnsigned 
{
	static_assert(false, "Unsupported data type.");
};

template <> struct TMakeUnsigned< int8>  { using Type = uint8; };
template <> struct TMakeUnsigned<uint8>  { using Type = uint8; };
template <> struct TMakeUnsigned< int16> { using Type = uint16; };
template <> struct TMakeUnsigned<uint16> { using Type = uint16; };
template <> struct TMakeUnsigned< int32> { using Type = uint32; };
template <> struct TMakeUnsigned<uint32> { using Type = uint32; };
template <> struct TMakeUnsigned< int64> { using Type = uint64; };
template <> struct TMakeUnsigned<uint64> { using Type = uint64; };

template <> struct TMakeUnsigned<  long>  { using Type = unsigned long; };
