#pragma once
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlArray.h"

/**
 * Mutable UTF-16 name with fixed inline storage.
 * Capacity includes the null terminator: FKrnlName<7> can hold L"ABCDEF".
 */
template <size_t InCapacity>
class FKrnlName
{
public:
	static constexpr size_t Capacity = InCapacity;
	static constexpr size_t MaxNameLength = Capacity - 1;

	static_assert(Capacity > 0, "A name needs space for its null terminator.");
	static_assert(Capacity <= 0x7fffffff / sizeof(WIDECHAR),
		"Capacity must fit the array's element count and byte-size limits.");

	using ElementType = WIDECHAR;
	using AllocatorType = TKrnlFixedAllocator<ElementType, static_cast<int32>(Capacity), int32>;
	using SizeType = typename AllocatorType::SizeType;

private:
	typedef TKrnlArray<EPoolType::NonPaged, ElementType, AllocatorType> DataType;

public:
	FORCEINLINE FKrnlName() noexcept
	{
		const EContainerStatus Status = Data.Initialize(static_cast<SizeType>(Capacity));
		NT_ASSERT(Status == EContainerStatus::Success);
		const SizeType Index = Data.Push(L'\0');
		NT_ASSERT(Index == 0);
	}

	FORCEINLINE FKrnlName(const FKrnlName& Other) noexcept
		: FKrnlName()
	{
		CopyFrom(Other);
	}

	FORCEINLINE FKrnlName& operator=(const FKrnlName& Other) noexcept
	{
		if (this != &Other)
		{
			CopyFrom(Other);
		}
		return *this;
	}

	// Inline storage cannot be transferred, so moving copies without allocating.
	FORCEINLINE FKrnlName(FKrnlName&& Other) noexcept
		: FKrnlName()
	{
		CopyFrom(Other);
	}

	FORCEINLINE FKrnlName& operator=(FKrnlName&& Other) noexcept
	{
		if (this != &Other)
		{
			CopyFrom(Other);
		}
		return *this;
	}

	/** Copies CharacterCount code units; Source need not be null terminated. */
	NODISCARD FORCEINLINE bool TryAssign(const WIDECHAR* Source, size_t CharacterCount) noexcept
	{
		if (CharacterCount > MaxNameLength || (CharacterCount != 0 && Source == nullptr))
		{
			return false;
		}

		for (size_t Index = 0; Index < CharacterCount; ++Index)
		{
			if (Source[Index] == L'\0')
			{
				return false;
			}
		}

		const SizeType NewNum = static_cast<SizeType>(CharacterCount + 1);
		const SizeType OldNum = Data.Num();
		if (NewNum > OldNum && !GrowTo(NewNum))
		{
			return false;
		}

		WIDECHAR* Destination = Data.GetData();
		if (CharacterCount != 0)
		{
			RtlMoveMemory(Destination, Source, CharacterCount * sizeof(WIDECHAR));
		}
		if (NewNum < OldNum)
		{
			const bool bResized = Data.SetNumUninitialized(NewNum, false);
			NT_ASSERT(bResized);
		}
		Destination[CharacterCount] = L'\0';
		return true;
	}

	/** Accepts any null-terminated name that fits within Capacity. */
	NODISCARD FORCEINLINE bool TryAssignCString(const WIDECHAR* Source) noexcept
	{
		if (Source == nullptr)
		{
			return false;
		}

		for (size_t Index = 0; Index < Capacity; ++Index)
		{
			if (Source[Index] == L'\0')
			{
				return TryAssign(Source, Index);
			}
		}
		return false;
	}

	NODISCARD FORCEINLINE bool TrySetChar(size_t Index, WIDECHAR Character) noexcept
	{
		if (Index >= Len() || Character == L'\0')
		{
			return false;
		}
		Data.GetData()[Index] = Character;
		return true;
	}

public:
	NODISCARD FORCEINLINE const WIDECHAR* operator*() const noexcept { return Data.GetData(); }
	NODISCARD FORCEINLINE const WIDECHAR* GetData() const noexcept { return Data.GetData(); }
	NODISCARD FORCEINLINE size_t Len() const noexcept { return static_cast<size_t>(Data.Num() - 1); }
	NODISCARD FORCEINLINE bool IsEmpty() const noexcept { return Data.Num() == 1; }
	NODISCARD static constexpr size_t GetCapacity() noexcept { return Capacity; }

public:
	/** The argument types enforce matching capacities. */
	static FORCEINLINE void Swap(FKrnlName& First, FKrnlName& Second) noexcept
	{
		if (&First == &Second)
		{
			return;
		}

		const size_t FirstLength = First.Len();
		const size_t SecondLength = Second.Len();
		const size_t LastIndex = FirstLength > SecondLength ? FirstLength : SecondLength;
		const SizeType MaxNum = static_cast<SizeType>(LastIndex + 1);
		const bool bFirstReady = First.GrowTo(MaxNum);
		const bool bSecondReady = Second.GrowTo(MaxNum);
		NT_ASSERT(bFirstReady && bSecondReady);

		WIDECHAR* FirstData = First.Data.GetData();
		WIDECHAR* SecondData = Second.Data.GetData();
		for (size_t Index = 0; Index <= LastIndex; ++Index)
		{
			if (Index <= FirstLength && Index <= SecondLength)
			{
				const WIDECHAR Temp = FirstData[Index];
				FirstData[Index] = SecondData[Index];
				SecondData[Index] = Temp;
			}
			else if (Index <= FirstLength)
			{
				SecondData[Index] = FirstData[Index];
			}
			else
			{
				FirstData[Index] = SecondData[Index];
			}
		}

		const bool bFirstSized = First.Data.SetNumUninitialized(static_cast<SizeType>(SecondLength + 1), false);
		const bool bSecondSized = Second.Data.SetNumUninitialized(static_cast<SizeType>(FirstLength + 1), false);
		NT_ASSERT(bFirstSized && bSecondSized);
	}

	NODISCARD FORCEINLINE bool operator==(const FKrnlName& Other) const noexcept
	{
		if (Len() != Other.Len())
		{
			return false;
		}
		for (size_t Index = 0; Index < Len(); ++Index)
		{
			if (GetData()[Index] != Other.GetData()[Index])
			{
				return false;
			}
		}
		return true;
	}

	NODISCARD FORCEINLINE bool operator!=(const FKrnlName& Other) const noexcept
	{
		return !(*this == Other);
	}

private:
	FORCEINLINE bool GrowTo(SizeType NewNum) noexcept
	{
		while (Data.Num() < NewNum)
		{
			if (Data.Push(L'\0') == INDEX_NONE)
			{
				return false;
			}
		}
		return true;
	}

	FORCEINLINE void CopyFrom(const FKrnlName& Other) noexcept
	{
		const bool bAssigned = TryAssign(Other.GetData(), Other.Len());
		NT_ASSERT(bAssigned);
	}

private:
	DataType Data;
};
