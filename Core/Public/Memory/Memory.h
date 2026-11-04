#pragma once	

#include "Core/Public/Misc/Defines.h"
#include "Core/Public/Misc/Types.h"
#include "Core/Public/Misc/Assertion.h"
#include "Core/Public/Templates/Template.h"

#include <string.h>
#include <new.h>


namespace Core::Memory
{
	enum class EMemoryType
	{
		Default,
		Kernel,
	};

	template <EMemoryType MemoryType>
	struct TMemory
	{
		static FORCEINLINE void* Memcpy(void* Dest, const void* Src, size_t Size)
		{
			return memcpy(Dest, Src, Size);
		}

		static FORCEINLINE void* Memzero(void* Dest, size_t Size)
		{
			return memset(Dest, 0, Size);
		}

		static FORCEINLINE void* Memset(void* Dest, uint8 Char, size_t Size)
		{
			return memset(Dest, Char, Size);
		}

		static FORCEINLINE void* Memmove(void* Dest, void* Src, size_t Size)
		{
			return memmove(Dest, Src, Size);
		}
	};

	using FDefaultMemory = TMemory<EMemoryType::Default>;
}

using FMemory = Core::Memory::FDefaultMemory;

template <typename ElementType, typename... ArgsType>
FORCEINLINE void ConstructItem(ElementType* Elements, ArgsType&&... Args)
{
	::new (Elements) ElementType(Forward<ArgsType>(Args)...);
}

template <typename ElementType, typename SizeType = int32>
FORCEINLINE void ConstructItems(ElementType* Elements, SizeType Count)
{
	if constexpr (TIsSigned<SizeType>::Value)
	{
		CORE_CHECK(Count >= 0);
	}

	if constexpr (TIsTriviallyRelocatable<ElementType>::Value)
	{
		FMemory::Memzero(Elements, Count * sizeof(ElementType));
	}
	else
	{
		using USizeType = typename TMakeUnsigned<SizeType>::Type;
		for (USizeType Index = 0; Index < (USizeType)Count; ++Index)
		{
			::new (&Elements[Index]) ElementType();
		}
	}
}

template <typename DestElementType, typename SourceElementType, typename SizeType = int32>
FORCEINLINE void ConstructItemsFrom(void* Dest, const SourceElementType* const Source, SizeType Count) noexcept
{
	if constexpr (TIsSigned<SizeType>::Value)
	{
		CORE_CHECK(Count >= 0);
	}

	if constexpr (TIsBitwiseConstructible<DestElementType, SourceElementType>::Value)
	{
		FMemory::Memcpy(Dest, Source, sizeof(SourceElementType) * Count);
	}
	else
	{
		using USizeType = typename TMakeUnsigned<SizeType>::Type;
		for (uint32 Index = 0; Index < (USizeType)Count; ++Index)
		{
			new (&((DestElementType*)Dest)[Index]) DestElementType(Source[Index]);
		}
	}
}

template <typename DestElementType, typename SourceElementType, typename SizeType = int32>
FORCEINLINE void RelocateItemsFrom(void* Dest, SourceElementType* Source, SizeType Count) noexcept
{
	if constexpr (TIsSigned<SizeType>::Value)
	{
		CORE_CHECK(Count >= 0);
	}

	if (Count == 0)
	{
		return;
	}

	if constexpr (TIsSame<DestElementType, SourceElementType>::Value)
	{
		if constexpr (TIsTriviallyRelocatable<SourceElementType>::Value)
		{
			FMemory::Memmove(Dest, Source, sizeof(SourceElementType) * Count);
		}
		else
		{
			DestElementType* DestItems = static_cast<DestElementType*>(Dest);
			if (DestItems == Source)
			{
				return;
			}

			using USizeType = typename TMakeUnsigned<SizeType>::Type;
			const USizeType NumItems = static_cast<USizeType>(Count);
			if (reinterpret_cast<size_t>(DestItems) < reinterpret_cast<size_t>(Source))
			{
				for (USizeType Index = 0; Index < NumItems; ++Index)
				{
					::new (&DestItems[Index]) DestElementType(Forward<SourceElementType>(Source[Index]));
					Source[Index].~SourceElementType();
				}
			}
			else
			{
				for (USizeType Index = NumItems; Index > 0; --Index)
				{
					const USizeType ItemIndex = Index - 1;
					::new (&DestItems[ItemIndex]) DestElementType(Forward<SourceElementType>(Source[ItemIndex]));
					Source[ItemIndex].~SourceElementType();
				}
			}
		}
	}
	else if constexpr (TAnd<TIsBitwiseConstructible<DestElementType, SourceElementType>,
		TIsTriviallyDestructible<SourceElementType>>::Value)
	{
		static_assert(sizeof(DestElementType) == sizeof(SourceElementType),
			"Bitwise relocation requires equal element sizes");
		FMemory::Memmove(Dest, Source, sizeof(SourceElementType) * Count);
	}
	else
	{
		using USizeType = typename TMakeUnsigned<SizeType>::Type;
		for (USizeType Index = 0; Index < static_cast<USizeType>(Count); ++Index)
		{
			::new (&static_cast<DestElementType*>(Dest)[Index])
				DestElementType(Forward<SourceElementType>(Source[Index]));
			Source[Index].~SourceElementType();
		}
	}
}

template <typename ElementType>
FORCEINLINE void DestructItem(ElementType* Element) noexcept
{
	if constexpr (TNot<TIsTriviallyDestructible<ElementType>>::Value)
	{
		Element->ElementType::~ElementType();
	}
}

template <typename ElementType, typename SizeType = int32>
FORCEINLINE void DestructItems(ElementType* Elements, SizeType Count)
{
	if constexpr (TIsSigned<SizeType>::Value)
	{
		CORE_CHECK(Count >= 0);
	}

	if constexpr (TNot<TIsTriviallyDestructible<ElementType>>::Value)
	{
		using USizeType = typename TMakeUnsigned<SizeType>::Type;
		for (USizeType Index = 0; Index < (USizeType)Count; ++Index)
		{
			(&Elements[Index])->ElementType::~ElementType();
		}
	}
}

/**
 * 생성자를 호출할건가?
 * 소멸자를 호출할건가?
 * move operator &&를 사용할건가?
 * copy operator &를 사용할건가?
 *
 * move assignment operator &&를 사용할건가?
 * copy assignment operator &를 사용할건가?
 *
 * tempalte <typename DestElementType, typename SrouceElementType, typename SizeType>
 * MoveFrom(DestElementType, SourceElementType, SizeType);
 *
 * tempalte <typename DestElementType, typename SrouceElementType, typename SizeType>
 * CopyFrom(DestElementType, SourceElementType, SizeType);
 */
