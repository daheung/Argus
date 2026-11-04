#pragma once

#include "Core/Public/Misc/Types.h"

template <typename T>
struct TIsSigned
{
	static_assert(false, "Unsupported data type.");
};

template <> struct TIsSigned<  int8> { enum { Value = true  }; };
template <> struct TIsSigned< uint8> { enum { Value = false }; };
template <> struct TIsSigned< int16> { enum { Value = true  }; };
template <> struct TIsSigned<uint16> { enum { Value = false }; };
template <> struct TIsSigned< int32> { enum { Value = true  }; };
template <> struct TIsSigned<uint32> { enum { Value = false }; };
template <> struct TIsSigned< int64> { enum { Value = true  }; };
template <> struct TIsSigned<uint64> { enum { Value = false }; };