#pragma once

#include "Kernel/Public/Containers/KrnlContainerStatus.h"
#include "Kernel/Public/Containers/KrnlContainerAllocator.h"
#include "Core/Public/Math/NumericLimits.h"
#include "Core/Public/Memory/Memory.h"


namespace Argus::Array
{
	template <typename FromArrayType, typename ToArrayType>
	constexpr static FORCEINLINE bool CanMoveTArrayPointersBetweenArrayTypes() noexcept
	{
		typedef typename FromArrayType::AllocatorType	FromAllocatorType;
		typedef typename ToArrayType::AllocatorType		ToAllocatorType;
		typedef typename FromArrayType::ElementType		FromElementType;
		typedef typename ToArrayType::ElementType		ToElementType;

		if constexpr (TIsSame<FromAllocatorType, ToAllocatorType>::Value)
		{
			return
				TOr<
					TIsSame<FromElementType, ToElementType>,
					TIsBitwiseConstructible<FromElementType, ToElementType>
				>::Value;
		}
		else
		{
			return false;
		}
	}

}

template <typename ContainerType, typename ElementType, typename SizeType>
class TIndexedContainerIterator
{
public:
	FORCEINLINE TIndexedContainerIterator(ContainerType& InContainerType, SizeType StartIndex = 0)
		: Container(&InContainerType)
		, Index(StartIndex)
	{
	}

public:
	FORCEINLINE TIndexedContainerIterator& operator++() noexcept
	{
		++Index;
		return *this;
	}

	FORCEINLINE TIndexedContainerIterator& operator++(int) noexcept
	{
		TIndexedContainerIterator Tmp(*this);
		++Index;
		return Tmp;
	}

	FORCEINLINE TIndexedContainerIterator& operator--() noexcept
	{
		--Index;
		return (*this);
	}

	FORCEINLINE bool operator==(const TIndexedContainerIterator& Other) const noexcept
	{
		return ((Container == Other.Container) && (Index == Other.Index));
	}

	FORCEINLINE bool operator!=(const TIndexedContainerIterator& Other) const noexcept
	{
		return !(operator==(Other));
	}

	FORCEINLINE ElementType& operator*()
	{
		return (*Container)[Index];
	}

	FORCEINLINE ElementType* operator->()
	{
		return &(*Container)[Index];
	}

private:
	ContainerType* Container;
	SizeType Index;
};

template <EPoolType InPoolType, typename InElementType, typename InAllocatorType>
class TKrnlArray
{
public:
	static constexpr EPoolType PoolType = InPoolType;

	template <EPoolType OtherInPoolType, typename OtherInElementType, typename OtherInAllocatorType>
	friend class TKrnlArray;

	typedef InAllocatorType AllocatorType;
	typedef InElementType ElementType;
	typedef typename AllocatorType::SizeType SizeType;

	typedef TIndexedContainerIterator<TKrnlArray<PoolType, ElementType, AllocatorType>, ElementType, SizeType> TIterator;

private:
	typedef typename TMakeUnsigned<SizeType>::Type USizeType;

public:
	FORCEINLINE TKrnlArray() noexcept
		: ArrayNum(0)
		, ArrayMax(0)
	{}

	FORCEINLINE explicit TKrnlArray(uint32 InPoolTag) noexcept
		: ArrayNum(0)
		, ArrayMax(0)
		, Allocator(InPoolTag)
	{}

	~TKrnlArray() noexcept
	{
		DestructItems(Allocator.GetAllocation(), ArrayNum);
		Allocator.Deallocate();
	}

	FORCEINLINE TKrnlArray(TKrnlArray&& Other) noexcept
		: ArrayMax(Other.ArrayMax)
		, ArrayNum(Other.ArrayNum)
		, Allocator(MoveTempIfPossible(Other.Allocator))
	{
		Other.ArrayMax = 0;
		Other.ArrayNum = 0;
	}

	// All operations that may allocate are explicit so callers can handle failure.
	NODISCARD EContainerStatus Initialize(SizeType InitialCapacity) noexcept
	{
		if (ArrayNum != 0 || ArrayMax != 0)
		{
			return EContainerStatus::InvalidState;
		}
		if constexpr (TIsSigned<SizeType>::Value)
		{
			if (InitialCapacity < 0)
			{
				return EContainerStatus::InvalidParameter;
			}
		}

		constexpr USizeType MaxSize = TIsSigned<SizeType>::Value
			? (static_cast<USizeType>(INDEX_NONE) >> 1)
			:  static_cast<USizeType>(INDEX_NONE);
		if (static_cast<USizeType>(InitialCapacity) > MaxSize / sizeof(ElementType))
		{
			return EContainerStatus::SizeOverflow;
		}

		return ResizeAllocation(InitialCapacity) ? EContainerStatus::Success : EContainerStatus::AllocationFailed;
	}

	NODISCARD EContainerStatus InitializeFromCopy(const ElementType* Elements, SizeType Count) noexcept
	{
		//if (Count != 0 && Elements == nullptr)
		//{
		//	return EContainerStatus::InvalidParameter;
		//}

		//EContainerStatus Status = Initialize(Count);
		//if (Status != EContainerStatus::Success)
		//{
		//	return Status;
		//}
		//if (Count != 0)
		//{
		//	ConstructItemsFrom<ElementType, ElementType>(GetData(), Elements, Count);
		//}
		//ArrayNum = Count;

		//return EContainerStatus::Success;

		return this->CopyToEmpty(Elements, Count);
	}

	template <typename OtherArrayType>
	NODISCARD EContainerStatus InitializeFromMove(OtherArrayType& Other) noexcept
	{
		return this->MoveToEmpty(Other);
	}

private:
	template <typename OtherElementType, typename OtherSizeType>
	FORCEINLINE EContainerStatus CopyToEmpty(const OtherElementType* OtherData, OtherSizeType OtherNum) noexcept
	{
		static_assert(sizeof(SizeType) >= sizeof(OtherSizeType), "Other Array function's SizeType is not compatible.");
		if constexpr (TNot<TIsBitwiseConstructible<ElementType, OtherElementType>>::Value)
		{
			return EContainerStatus::CheckFailed;
		}

		if (OtherNum != 0 && OtherData == nullptr)
		{
			return EContainerStatus::InvalidParameter;
		}

		EContainerStatus Status = Initialize(OtherNum);
		if (Status != EContainerStatus::Success)
		{
			return Status;
		}
		if (OtherNum != 0)
		{
			ConstructItemsFrom<ElementType, OtherElementType>(this->GetData(), OtherData, OtherNum);
		}
		ArrayNum = OtherNum;

		return EContainerStatus::Success;
	}

	template <typename OtherElementType, typename OtherSizeType>
	FORCEINLINE EContainerStatus CopyToEmptyWithSlack(const OtherElementType* OtherData, OtherSizeType OtherNum, SizeType ExtraSlack) noexcept
	{
		static_assert(sizeof(SizeType) >= sizeof(OtherSizeType), "Other Array function's SizeType is not compatible.");
		if constexpr (TNot<TIsBitwiseConstructible<ElementType, OtherElementType>>::Value)
		{
			return EContainerStatus::CheckFailed;
		}

		if (OtherNum <= 0 || OtherData == nullptr || ExtraSlack < 0)
		{
			return EContainerStatus::InvalidParameter;
		}

		EContainerStatus Status = Initialize(OtherNum);
		if (Status != EContainerStatus::Success)
		{
			return Status;
		}
		
		const SizeType NewMax = this->ArrayNum + ExtraSlack;
		const bool bResizeSucceed = ResizeAllocation(NewMax);
		if (!bResizeSucceed)
		{
			return EContainerStatus::AllocationFailed;
		}

		if (OtherNum != 0)
		{
			ConstructItemsFrom<ElementType, OtherElementType>(this->GetData(), OtherData, OtherNum);
		}
		ArrayNum = OtherNum;

		return EContainerStatus::Success;
	}

	template <typename OtherArrayType>
	EContainerStatus MoveToEmpty(OtherArrayType& Other) noexcept
	{
		constexpr EPoolType OtherPoolType = OtherArrayType::PoolType;
		static_assert((PoolType == OtherPoolType) ||
			(PoolType == EPoolType::Paged && OtherPoolType == EPoolType::NonPaged),
			"TKrnlArray move requires the same pool type or a NonPaged source with a Paged destination");

		if (static_cast<const void*>(this) == static_cast<const void*>(&Other))
		{
			return EContainerStatus::InvalidParameter;
		}
		if (ArrayNum != 0 || ArrayMax != 0)
		{
			return EContainerStatus::InvalidState;
		}

		using OtherSizeType = typename OtherArrayType::SizeType;
		if (Other.ArrayNum < 0 || Other.ArrayMax < Other.ArrayNum)
		{
			return EContainerStatus::InvalidParameter;
		}

		const SizeType NewNum = static_cast<SizeType>(Other.ArrayNum);
		if (NewNum < 0)
		{
			return EContainerStatus::SizeOverflow;
		}
		if (static_cast<OtherSizeType>(NewNum) != Other.ArrayNum)
		{
			return EContainerStatus::SizeOverflow;
		}

		using OtherElementType = typename OtherArrayType::ElementType;
		if constexpr (TIsSame<ElementType, OtherElementType>::Value)
		{
			if (this->TryMoveAllocationFromOther(Other))
			{
				return EContainerStatus::Success;
			}
		}

		EContainerStatus Status = Initialize(NewNum);
		if (Status != EContainerStatus::Success)
		{
			return Status;
		}
		if (NewNum != 0)
		{
			RelocateItemsFrom<ElementType, OtherElementType>(GetData(), Other.GetData(), NewNum);
		}

		ArrayNum = NewNum;
		Other.Allocator.Deallocate();
		Other.ArrayNum = 0;
		Other.ArrayMax = 0;

		return EContainerStatus::Success;
	}

	template <typename FromArrayType, typename ToArrayType>
	static FORCEINLINE EContainerStatus MoveOrCopy(ToArrayType& ToArray, FromArrayType& FromArray) noexcept
	{
		if constexpr (Argus::Array::CanMoveTArrayPointersBetweenArrayTypes<FromArrayType, ToArrayType>())
		{
			// Move
			using FromAllocatorType = typename FromArrayType::AllocatorType;
			using ToAllocatorType   = typename ToArrayType::AllocatorType;

			EContainerStatus Status = ToArray.Allocator.MoveToEmpty(FromArray.Allocator);
			if (Status == EContainerStatus::Success)
			{
				if constexpr (sizeof(USizeType) < sizeof(typename FromArrayType::USizeType))
				{
					CORE_CHECK(ToArray.ArrayNum == FromArray.ArrayNum && ToArray.ArrayMax == FromArray.ArrayMax);
				}
			}

			return Status;
		}
		else
		{
			// Copy
			return ToArray.CopyToEmpty(FromArray.GetData(), FromArray.GetNum());
		}
	}

public:
	ElementType& At(SizeType Index) noexcept
	{
		CORE_CHECK(Index < ArrayNum);
		return this->GetAllocation()[Index];
	}

	const ElementType& At(SizeType Index) const noexcept
	{
		CORE_CHECK(Index < ArrayNum);
		return this->GetAllocation()[Index];
	}

	FORCEINLINE		  ElementType& Last()		noexcept { return this->At(this->ArrayNum - 1); }
	FORCEINLINE const ElementType& Last() const noexcept { return this->At(this->ArrayNum - 1); }

	FORCEINLINE		  ElementType& operator[](SizeType Index)	    noexcept { return At(Index); }
	FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept { return At(Index); }

	NODISCARD FORCEINLINE SizeType Num() const noexcept { return this->ArrayNum; }
	NODISCARD FORCEINLINE SizeType Max() const noexcept { return this->ArrayMax; }

	FORCEINLINE void CheckInvariants() const { CORE_CHECK((ArrayNum >= 0) & (ArrayMax >= ArrayNum)); }
	FORCEINLINE bool CheckIndex(SizeType Index) const { return 0 <= Index && Index < this->ArrayNum; }

	FORCEINLINE bool IsEmpty() const noexcept
	{
		return ArrayNum == 0;
	}

	FORCEINLINE size_t GetAllocatedSize() const noexcept { return this->Allocator.GetAllocatedSize(this->ArrayMax, sizeof(ElementType)); }

	FORCEINLINE		  ElementType* GetAllocation()	     noexcept { return this->Allocator.GetAllocation(); }
	FORCEINLINE const ElementType* GetAllocation() const noexcept { return this->Allocator.GetAllocation(); }

	FORCEINLINE		  ElementType* GetData()	   noexcept { return this->Allocator.GetAllocation(); }
	FORCEINLINE const ElementType* GetData() const noexcept { return this->Allocator.GetAllocation(); }

	FORCEINLINE SizeType GetNum() const noexcept { return this->ArrayNum; }
	FORCEINLINE SizeType GetMax() const noexcept { return this->ArrayMax; }

	NODISCARD FORCEINLINE SizeType Slack() const noexcept
	{
		return this->ArrayMax - this->ArrayNum;
	}

	NODISCARD FORCEINLINE uint32 Tag() const noexcept 
	{ 
		return this->Allocator.GetPoolTag(); 
	}

public:
	/**
	 * @brief Destroys all elements and optionally resizes the allocation.
	 * @param Slack Requested capacity after clearing. Zero keeps the current allocation.
	 * @return Success if the elements were cleared and any requested resize succeeded;
	 *         InvalidParameter if Slack is negative (the array is unchanged);
	 *         AllocationFailed if resizing fails. On AllocationFailed, all elements
	 *         have already been destroyed and ArrayNum is zero; the allocation
	 *         and ArrayMax retain their pre-resize state.
	 */
	FORCEINLINE EContainerStatus Empty(SizeType Slack = 0) noexcept
	{
		if (Slack < 0)
		{
			return EContainerStatus::InvalidParameter;
		}

		DestructItems<ElementType>(GetAllocation(), this->ArrayNum);
		this->ArrayNum = 0;

		if (Slack)
		{
			bool bResizeSucceed = this->ResizeAllocation(Slack);
			if (!bResizeSucceed)
			{
				return EContainerStatus::AllocationFailed;
			}
		}

		return EContainerStatus::Success;
	}

	FORCEINLINE bool Shrink() noexcept
	{
		CheckInvariants();

		if (this->ArrayNum != this->ArrayMax)
		{
			return ResizeAllocation(ArrayNum);
		}

		return true;
	}

	FORCEINLINE bool ResizeGrow(SizeType Count = 1) noexcept
	{
		if constexpr (TIsSigned<SizeType>::Value)
		{
			if (Count < 0)
			{
				return false;
			}
		}

		if (Count == 0)
		{
			return true;
		}

		constexpr USizeType MaxSize = TIsSigned<SizeType>::Value
			? (static_cast<USizeType>(INDEX_NONE) >> 1)
			:  static_cast<USizeType>(INDEX_NONE);
		const USizeType CurrentNum = static_cast<USizeType>(ArrayNum);
		const USizeType Additional = static_cast<USizeType>(Count);
		if (Additional > MaxSize - CurrentNum)
		{
			return false;
		}

		const USizeType Required = CurrentNum + Additional;
		const USizeType CurrentMax = static_cast<USizeType>(ArrayMax);
		if (Required <= CurrentMax)
		{
			return true;
		}

		const USizeType NewMax = Argus::Allocator::CalculateGrowCapacity(CurrentMax, Required, MaxSize);
		return ResizeAllocation(static_cast<SizeType>(NewMax));
	}

	FORCEINLINE bool ResizeAllocation(SizeType NewMax) noexcept
	{
		static_assert(TIsTriviallyRelocatable<ElementType>::Value,
			"TKrnlArray requires bitwise-relocatable elements because reallocation moves live elements as bytes");

		if constexpr (TIsSigned<SizeType>::Value)
		{
			if (NewMax < 0)
			{
				return false;
			}
		}

		if (NewMax < ArrayNum)
		{
			return false;
		}
		if (NewMax == ArrayMax)
		{
			return NewMax == 0 || GetAllocation() != nullptr;
		}

		if (NewMax == 0)
		{
			Allocator.Deallocate();
			ArrayMax = 0;
			return true;
		}

		if (Allocator.Reallocate(ArrayNum, NewMax, sizeof(ElementType), alignof(ElementType)) == nullptr)
		{
			return false;
		}

		ArrayMax = NewMax;
		return true;
	}

public:
	FORCEINLINE SizeType Push(const ElementType& Element) noexcept
	{
		SizeType Index = PushUninitialized();
		if (Index == INDEX_NONE)
		{
			return Index;
		}

		ConstructItem(&GetAllocation()[Index], Element);

		return Index;
	}

	FORCEINLINE SizeType Push(ElementType&& Element) noexcept
	{
		SizeType Index = PushUninitialized();
		if (Index == INDEX_NONE)
		{
			return Index;
		}

		ConstructItem(&GetAllocation()[Index], Forward<ElementType>(Element));

		return Index;
	}

	FORCEINLINE SizeType Insert(const ElementType& Item, SizeType Index) noexcept
	{
		const bool bSucceed = InsertUninitializedImpl(Index, 1);
		if (!bSucceed)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		new (GetData() + Index) ElementType(Item);
		return Index;
	}

	FORCEINLINE SizeType Insert(ElementType&& Item, SizeType Index) noexcept
	{
		const bool bSucceed = InsertUninitializedImpl(Index, 1);
		if (!bSucceed)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		new (GetData() + Index) ElementType(Forward<ElementType>(Item));
		return Index;
	}

	FORCEINLINE SizeType Insert(const ElementType* Ptr, SizeType Count, SizeType Index) noexcept
	{
		const bool bSucceed = InsertUninitializedImpl(Index, Count);
		if (!bSucceed)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		ConstructItemsFrom<ElementType>(GetData() + Index, Ptr, Count);
		return Index;
	}

	FORCEINLINE ElementType Pop(bool bAllowShrinking = true) noexcept
	{
		CORE_CHECK(ArrayNum > 0);
		ElementType PopResult = MoveTempIfPossible(GetAllocation()[ArrayNum - 1]);
		RemoveAtImpl(ArrayNum - 1, 1, bAllowShrinking);
		return PopResult;
	}

	FORCEINLINE TKrnlArray Pop(SizeType Count, bool bAllowShrinking = true, bool* bAllocationFailed = nullptr) noexcept
	{
		if (bAllocationFailed != nullptr)
		{
			*bAllocationFailed = false;
		}

		TKrnlArray OutArray(this->Tag());
		if  constexpr (TIsSigned<SizeType>::Value)
		{
			if (Count < 0)
			{
				return OutArray;
			}
		}
		if (Count > ArrayNum || Count == 0)
		{
			return OutArray;
		}
		if (OutArray.Initialize(Count) != EContainerStatus::Success)
		{
			if (bAllocationFailed != nullptr)
			{
				*bAllocationFailed = true;
			}
			return OutArray;
		}

		ElementType* Start = &this->GetAllocation()[this->ArrayNum - Count];
		RelocateItemsFrom<ElementType>(OutArray.GetAllocation(), Start, Count);

		this->ArrayNum -= Count;
		OutArray.ArrayNum = Count;

		if (bAllowShrinking)
		{
			Shrink();
		}

		return OutArray;
	}

	FORCEINLINE bool PopAt(SizeType Index, ElementType& OutElement) noexcept
	{
		if (Index < 0)
		{
			return false;
		}
		if (Index >= this->ArrayNum)
		{
			return false;
		}

		OutElement = MoveTempIfPossible(this->GetAllocation()[Index]);
		RemoveAtImpl(Index);

		return true;
	}

public:
	/**
	 * @brief Removes the element at Index and shifts later elements left to preserve order.
	 * @param Index Zero-based index of the element to remove.
	 * @return true if an element was removed; false if Index is out of range.
	 *
	 * An invalid index leaves the array unchanged. For a valid index, removal
	 * destroys the element and relocates later elements without allocating or
	 * shrinking storage, so there is no recoverable failure path. The element's
	 * destructor and relocation must complete safely; bool reports index
	 * validity, not failures in those operations.
	 */
	FORCEINLINE bool RemoveAt(SizeType Index) noexcept
	{
		if (Index < 0)
		{
			return false;
		}
		if (Index >= this->ArrayNum)
		{
			return false;
		}

		this->CheckInvariants();
		this->RemoveAtImpl(Index);

		return true;
	}

	/**
	 * @brief Removes Count elements at Index while preserving the order of later elements.
	 * @param Index First index to remove; may equal ArrayNum only when Count is zero.
	 * @param Count Number of elements to remove. Zero leaves the array unchanged.
	 * @return true for a valid range, including an empty range; false otherwise.
	 */
	FORCEINLINE bool RemoveAt(SizeType Index, SizeType Count, bool bAllowShrinking = true) noexcept
	{
		if (Index < 0 || Count < 0 || Index > this->ArrayNum)
		{
			return false;
		}
		if (Count > this->ArrayNum - Index)
		{
			return false;
		}
		if (Count == 0)
		{
			return true;
		}

		this->CheckInvariants();
		this->RemoveAtImpl(Index, Count, bAllowShrinking);

		return true;
	}

	FORCEINLINE bool RemoveAtLast(bool bAllowShrinking = true) noexcept
	{
		if (IsEmpty())
		{
			return false;
		}

		return this->RemoveAtImpl(this->ArrayNum - 1, 1, bAllowShrinking);
	}

	FORCEINLINE bool RemoveAtLast(SizeType Count = 1, bool bAllowShrinking = true) noexcept
	{
		if (this->ArrayNum < Count)
		{
			return false;
		}

		return this->RemoveAtImpl(this->ArrayNum - Count, Count, bAllowShrinking);
	}

private:
	void RemoveAtImpl(SizeType Index) noexcept
	{
		ElementType* Start = &GetAllocation()[Index];
		DestructItem<ElementType>(Start);

		SizeType NumToMove = (this->ArrayNum - Index) - 1;
		if (NumToMove > 0)
		{
			RelocateItemsFrom<ElementType>(Start, Start + 1, NumToMove);
		}
		--this->ArrayNum;
	}

	void RemoveAtImpl(SizeType Index, SizeType Count, bool bAllowShrinking) noexcept
	{
		ElementType* Start = &GetAllocation()[Index];
		DestructItems<ElementType, SizeType>(Start, Count);

		SizeType NumToMove = (this->ArrayNum - Index) - Count;
		if (NumToMove > 0)
		{
			RelocateItemsFrom<ElementType>(Start, Start + Count, NumToMove);
		}
		this->ArrayNum -= Count;

		if (bAllowShrinking)
		{
			this->Shrink();
		}
	}

public:
	FORCEINLINE bool Find(const ElementType& Element, SizeType& Index) const noexcept
	{
		Index = this->Find(Element);
		return Index != INDEX_NONE;
	}

	FORCEINLINE SizeType Find(const ElementType& Element) const noexcept
	{
		const ElementType* Start = this->GetAllocation();
		for (const ElementType* Data = Start; Data != Start + ArrayNum; ++Data)
		{
			if (*Data == Element)
			{
				return static_cast<SizeType>(Data - Start);
			}
		}

		return INDEX_NONE;
	}

	template <typename Predicate>
	FORCEINLINE SizeType FindIndexByPredicate(Predicate Pred) const noexcept
	{
		const ElementType* Start = this->GetAllocation();
		for (const ElementType* Data = Start; Data != Start + ArrayNum; ++Data)
		{
			if (::Invoke(Pred, *Data))
			{
				return static_cast<SizeType>(Data - Start);
			}
		}

		return INDEX_NONE;
	}

	FORCEINLINE SizeType PushUninitialized(SizeType Count = 1)
	{
		if (this->ArrayNum < 0 || Count < 0)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		constexpr USizeType MaxNum = static_cast<USizeType>(TNumericLimits<SizeType>::Max());
		const USizeType CurrentNum = static_cast<USizeType>(this->ArrayNum);
		const USizeType Additional = static_cast<USizeType>(Count);
		if (CurrentNum > MaxNum || Additional > MaxNum - CurrentNum)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		const SizeType OldNum = this->ArrayNum;
		const SizeType NewNum = static_cast<SizeType>(CurrentNum + Additional);
		if (ArrayMax < NewNum)
		{
			const bool bResizeSucceed = ResizeGrow(Count);
			if (!bResizeSucceed)
			{
				return static_cast<SizeType>(INDEX_NONE);
			}
		}

		ArrayNum = NewNum;
		return OldNum;
	}

	FORCEINLINE bool SetNumUninitialized(SizeType NewNum, bool bAllowShrinking = true) noexcept
	{
		if (NewNum < 0)
		{
			return false;
		}

		if (NewNum > Num())
		{
			ReturnIfFalse(PushUninitialized(NewNum - Num()) != INDEX_NONE);
		}
		else if (NewNum < Num())
		{
			ReturnIfFalse(RemoveAt(NewNum, Num() - NewNum, bAllowShrinking));
		}

		return true;
	}

	FORCEINLINE bool InsertUninitializedImpl(SizeType Index, SizeType Count)
	{
		const SizeType OldNum = ArrayNum;
		const SizeType NewNum = OldNum + Count;

		if ((USizeType)Count > (USizeType)ArrayMax - OldNum)
		{
			ReturnIfFalse(ResizeGrow(NewNum));
		}

		ElementType* Data = GetData() + Index;
		RelocateItemsFrom<ElementType>(Data + Count, Data, (OldNum - Index));
		ArrayNum = NewNum;

		return true;
	}

public:
	/**
	 * @brief Exchanges storage and element counts of two arrays of the same type.
	 * @param First First array to exchange. Passing the same array twice has no effect.
	 * @param Second Second array to exchange.
	 *
	 * Only linear allocators support this constant-time, non-failing operation.
	 * Live elements remain in their allocation blocks; none are moved or destroyed.
	 */
	template <typename ArrayTypeA, typename ArrayTypeB>
	static FORCEINLINE void Swap(ArrayTypeA& First, ArrayTypeB& Second) noexcept
	{
		using AllocatorTypeA = typename ArrayTypeA::AllocatorType;
		using AllocatorTypeB = typename ArrayTypeB::AllocatorType;

		static_assert(Argus::Allocator::TypeTraits::TCanSwapAllocationBlocks<AllocatorTypeA, AllocatorTypeB>::Value,
			"TKrnlArray::Swap requires compatible linear allocators");

		if (static_cast<const void*>(&First) == static_cast<const void*>(&Second))
		{
			return;
		}

		::Swap(First.Allocator, Second.Allocator);
		::Swap(First.ArrayNum, Second.ArrayNum);
		::Swap(First.ArrayMax, Second.ArrayMax);
	}

	template <typename ArrayTypeA, typename ArrayTypeB>
	static FORCEINLINE bool TrySwap(ArrayTypeA& First, ArrayTypeB& Second) noexcept
	{
		using AllocatorTypeA = typename ArrayTypeA::AllocatorType;
		using AllocatorTypeB = typename ArrayTypeB::AllocatorType;

		if constexpr (Argus::Allocator::TypeTraits::TCanSwapAllocationBlocks<AllocatorTypeA, AllocatorTypeB>::Value)
		{
			Swap<ArrayTypeA, ArrayTypeB>(First, Second);
			return true;
		}

		// ...
		return false;

		//return true;
	}

private:
	/**
	 * Tries to take ownership of another array's allocation without moving its elements.
	 * The element types must be identical: bitwise relocatability alone does not
	 * permit an existing object to become an object of a different type.
	 *
	 * Returns false without changing either array when the block cannot be taken,
	 * allowing the caller to fall back to element relocation. On success, this
	 * array owns the block and live elements, and Other becomes empty.
	 * Requires an empty destination and source counts representable by SizeType.
	 */
	template <typename OtherArrayType>
	bool TryMoveAllocationFromOther(OtherArrayType& Other) noexcept
	{
		using OtherAllocatorType = typename OtherArrayType::AllocatorType;
		using OtherSizeType		 = typename OtherArrayType::SizeType;
		if constexpr (!OtherAllocatorType::NeedsElementType)
		{
			return false;
		}
		else
		{
			using OtherElementType = typename OtherAllocatorType::ElementType;
			if constexpr (TNot<TIsSame<ElementType, OtherElementType>>::Value)
			{
				return false;
			}

			const SizeType OtherMax = static_cast<SizeType>(Other.ArrayMax);
			if (OtherMax < 0)
			{
				return false;
			}
			if (static_cast<OtherSizeType>(OtherMax) != Other.ArrayMax)
			{
				return false;
			}

			constexpr EPoolType OtherPoolType = OtherAllocatorType::PoolType;
			bool bSucceed = this->Allocator.MoveToEmptyFromOtherAllocator<OtherSizeType, OtherPoolType>(Other.Allocator);
			if (!bSucceed)
			{
				return false;
			}

			this->ArrayNum = static_cast<SizeType>(Other.ArrayNum);
			this->ArrayMax = OtherMax;

			Other.ArrayNum = 0;
			Other.ArrayMax = 0;

			return true;
		}
	}

public:
	NODISCARD FORCEINLINE TIterator Begin() noexcept
	{
		return TIterator(*this);
	}

	NODISCARD FORCEINLINE TIterator End() noexcept
	{
		return TIterator(*this, ArrayNum);
	}

private:
	SizeType ArrayNum;
	SizeType ArrayMax;
	AllocatorType Allocator;
};
