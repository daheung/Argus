#pragma once

#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/RemoveConstVolatile.h"
#include "Core/Public/Templates/RemoveReference.h"

namespace Core::Private::TypeTraits::Decay
{
	template <typename T>
	struct TDecayNonReference
	{
		typedef typename TRemoveConstVolatile<T>::Type Type;
	};

	template <typename T>
	struct TDecayNonReference<T[]>
	{
		typedef T* Type;
	};

	template <typename T, uint32 N>
	struct TDecayNonReference<T[N]>
	{
		typedef T* Type;
	};

	template <typename RetType, typename... Params>
	struct TDecayNonReference<RetType(Params...)>
	{
		typedef RetType(*Type)(Params...);
	};
}

template <typename T>
struct TDecay
{
	typedef typename Core::Private::TypeTraits::Decay::TDecayNonReference<typename TRemoveReference<T>::Type> ::Type Type;
};