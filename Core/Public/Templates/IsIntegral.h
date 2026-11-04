#pragma once

#include "Core/Public/Templates/IsAnyOf.h"

template <typename T>
struct TIsIntegral
{
	enum { 
		Value = TIsAnyOf<T,
			bool,
			char	 , unsigned char,
			char16_t,
			char32_t,
			wchar_t,
			short	 , unsigned short,
			int		 , unsigned int,
			long	 , unsigned long,
			long long, unsigned long long
		>::Value
	};
};