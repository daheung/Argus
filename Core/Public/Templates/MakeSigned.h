#pragma once

#include "Core/Public/Misc/Types.h"

template <typename T>
struct TMakeSigned 
{
	static_assert(false, "Unsupported data type.");
};

template <> struct TMakeSigned< int8>  { using Type = int8; };
template <> struct TMakeSigned<uint8>  { using Type = int8; };
template <> struct TMakeSigned< int16> { using Type = int16; };
template <> struct TMakeSigned<uint16> { using Type = int16; };
template <> struct TMakeSigned< int32> { using Type = int32; };
template <> struct TMakeSigned<uint32> { using Type = int32; };
template <> struct TMakeSigned< int64> { using Type = int64; };
template <> struct TMakeSigned<uint64> { using Type = int64; };