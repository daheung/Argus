#pragma once

#include "Kernel/Public/Containers/KrnlContainerAllocator.h"
#include "Kernel/Public/Containers/KrnlContainerStatus.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Core/Public/Memory/Memory.h"
#include "Core/Public/Templates/Atomic.h"

/**
 * Fixed-capacity single-producer, single-consumer queue.
 *
 * Construct the queue, call Initialize(Capacity), and check its result before
 * starting the producer or consumer. Constructors only set the pool tag and
 * indices; they do not allocate storage. Initialize allocates Capacity + 1
 * slots so Capacity elements can be queued while one slot remains empty to
 * distinguish the full and empty states.
 *
 * Push never expands storage. It returns INDEX_NONE while uninitialized or
 * full; the caller decides whether to drop, retry, or report the element.
 * Additional producers or consumers require external serialization.
 * Initialization, Reset, and destruction require both sides to be stopped.
 * The allocator owns raw storage; this queue owns the live elements.
 * 
 * Capacity = 2; storage has 3 slots.
 * H = HeadIndex (next write), T = TailIndex (next read).
 *
 *                     Slot1      Slot2      Slot3	     Tail    Head
 * Empty:           [  Empty  ][  Empty  ][  Empty  ]    0       0
 * After Push(A):   [ Element ][  Empty  ][  Empty  ]    0       1
 * After Push(B):   [ Element ][ Element ][  Empty  ]    0       2      FULL
 *
 * After Pop(A):    [  Empty  ][ Element ][  Empty  ]    1       2
 * After Push(C):   [  Empty  ][ Element ][ Element ]    1       0	    FULL
 *
 * Empty:      H  == T. 
 * Full : Next(H) == T.
 */
template <EPoolType InPoolType, typename InElementType, typename InAllocatorType>
class TKrnlSpscCircularQueue
{
public:
	static constexpr EPoolType PoolType = InPoolType;

	using ElementType	= InElementType;
	using AllocatorType = InAllocatorType;
	using SizeType		= typename AllocatorType::SizeType;

	static_assert(AllocatorType::NeedsElementType,
		"TKrnlSpscCircularQueue requires an element-aware allocator");
	static_assert(TIsSame<ElementType, typename AllocatorType::ElementType>::Value,
		"TKrnlSpscCircularQueue allocator element type must match ElementType");

	typedef TIndexedContainerIterator<TKrnlSpscCircularQueue<PoolType, ElementType, AllocatorType>, ElementType, SizeType> TIterator;

private:
	using USizeType = typename TMakeUnsigned<SizeType>::Type;

public:
	FORCEINLINE TKrnlSpscCircularQueue() noexcept
		: TailIndex(0)
		, HeadIndex(0)
		, QueueMax()
		, QueueAllocator()
	{
	}

	FORCEINLINE explicit TKrnlSpscCircularQueue(uint32 InPoolTag) noexcept
		: TailIndex(0)
		, HeadIndex(0)
		, QueueMax()
		, QueueAllocator(InPoolTag)
	{
	}

	~TKrnlSpscCircularQueue() noexcept
	{
		Reset();
	}

	TKrnlSpscCircularQueue(const TKrnlSpscCircularQueue&) = delete;
	TKrnlSpscCircularQueue& operator=(const TKrnlSpscCircularQueue&) = delete;
	TKrnlSpscCircularQueue(TKrnlSpscCircularQueue&&) = delete;
	TKrnlSpscCircularQueue& operator=(TKrnlSpscCircularQueue&&) = delete;

public:
	// Allocate the fixed storage once. Allocation failure leaves the queue unusable
	// until a later Initialize call succeeds; do not start producers before then.
	NODISCARD FORCEINLINE EContainerStatus Initialize(SizeType InitialCapacity) noexcept
	{
		if constexpr (TIsSigned<SizeType>::Value)
		{
			if (InitialCapacity <= 0)
			{
				return EContainerStatus::InvalidParameter;
			}
		}
		else if (InitialCapacity == 0)
		{
			return EContainerStatus::InvalidParameter;
		}

		if (QueueMax != 0)
		{
			return EContainerStatus::InvalidState;
		}

		constexpr USizeType MaxSize = TIsSigned<SizeType>::Value
			? (static_cast<USizeType>(INDEX_NONE) >> 1)
			:  static_cast<USizeType>(INDEX_NONE);
		// One extra storage slot distinguishes full from empty without growing counters.
		if ( sizeof(ElementType) > static_cast<uint64>(MaxSize) ||
			alignof(ElementType) > static_cast<uint64>(MaxSize) ||
			static_cast<uint64>(InitialCapacity) >= static_cast<uint64>(MaxSize) / sizeof(ElementType))
		{
			return EContainerStatus::SizeOverflow;
		}

		const SizeType StorageCapacity = InitialCapacity + static_cast<SizeType>(1);
		if (QueueAllocator.Allocate(StorageCapacity, static_cast<SizeType>(sizeof(ElementType)),
			static_cast<SizeType>(alignof(ElementType))) == nullptr)
		{
			return EContainerStatus::AllocationFailed;
		}

		QueueMax = InitialCapacity;

		return EContainerStatus::Success;
	}

	NODISCARD FORCEINLINE EContainerStatus InitializeFromCopy(const ElementType* Elements, SizeType Count) noexcept
	{
		if (Elements == nullptr)
		{
			return EContainerStatus::InvalidParameter;
		}

		const EContainerStatus Status = Initialize(Count);
		if (Status != EContainerStatus::Success)
		{
			return Status;
		}

		for (SizeType Index = 0; Index < Count; ++Index)
		{
			ConstructItem(&GetElements()[Index], Elements[Index]);
		}
		HeadIndex.store(static_cast<uint64>(Count), Atomic::EMemoryOrder::Release);

		return EContainerStatus::Success;
	}

	// Destroy queued elements and rewind both indices while retaining the
	// allocation and capacity. Call only after the producer and consumer stop.
	FORCEINLINE void Reset() noexcept
	{
		if (QueueMax == 0)
		{
			return;
		}

		const SizeType Head = HeadIndex.load(Atomic::EMemoryOrder::Relaxed);
		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Relaxed);
		for (SizeType Index = Tail; Index != Head; Index = NextIndex(Index))
		{
			DestructItem(&GetElements()[Index]);
		}

		TailIndex.store(0, Atomic::EMemoryOrder::Relaxed);
		HeadIndex.store(0, Atomic::EMemoryOrder::Relaxed);
	}

public:
	// Returns the physical slot in [0, QueueMax], or INDEX_NONE if uninitialized
	// or full. No queue storage allocation or automatic growth occurs here.
	FORCEINLINE SizeType Push(const ElementType& Element) noexcept
	{
		if (QueueMax == 0)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		const SizeType Head = HeadIndex.load(Atomic::EMemoryOrder::Relaxed);
		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);
		const SizeType NextHead = NextIndex(Head);
		if (NextHead == Tail)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		ConstructItem(&GetElements()[Head], Element);
		HeadIndex.store(NextHead, Atomic::EMemoryOrder::Release);
		return Head;
	}

	// On failure, Element has not been moved into the queue.
	FORCEINLINE SizeType Push(ElementType&& Element) noexcept
	{
		if (QueueMax == 0)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		const SizeType Head = HeadIndex.load(Atomic::EMemoryOrder::Relaxed);
		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);
		const SizeType NextHead = NextIndex(Head);
		if (NextHead == Tail)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		ConstructItem(&GetElements()[Head], Forward<ElementType>(Element));
		HeadIndex.store(NextHead, Atomic::EMemoryOrder::Release);
		return Head;
	}

	/**
	 * @brief Copies an element into the queue, evicting the oldest element if full.
	 * @param Element Element to copy; must not alias the object pointed to by OutElement.
	 * @param OutElement Optional pointer to an existing element that receives the
	 *                   evicted element by move assignment. If null, the evicted
	 *                   element is destroyed. If the queue is not full, this object
	 *                   remains unchanged.
	 * @return The physical insertion index, or INDEX_NONE if the queue is
	 *         uninitialized. A successful return does not indicate whether an
	 *         element was evicted.
	 * @warning Eviction advances TailIndex from the producer. Externally serialize
	 *          this call with consumers and other producers; this queue does not lock.
	 */
	FORCEINLINE SizeType PushDropOldestIfFull(const ElementType& Element, ElementType* const OutElement) noexcept
	{
		if (QueueMax == 0)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		const SizeType Head = HeadIndex.load(Atomic::EMemoryOrder::Relaxed);
		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);
		const SizeType NextHead = NextIndex(Head);
		if (NextHead == Tail)
		{
			ElementType* Slot = &GetElements()[Tail];

			if (OutElement)
			{
				*OutElement = MoveTempIfPossible(*Slot);
			}

			DestructItem(Slot);

			TailIndex.store(NextIndex(Tail), Atomic::EMemoryOrder::Release);
		}

		ConstructItem(&GetElements()[Head], Element);
		HeadIndex.store(NextHead, Atomic::EMemoryOrder::Release);
		return Head;
	}

	/**
	 * @brief Moves an element into the queue, evicting the oldest element if full.
	 * @param Element Element to move on success; must not alias the object pointed
	 *                to by OutElement.
	 * @param OutElement Optional pointer to an existing element that receives the
	 *                   evicted element by move assignment. If null, the evicted
	 *                   element is destroyed. If the queue is not full, this object
	 *                   remains unchanged.
	 * @return The physical insertion index, or INDEX_NONE if the queue is
	 *         uninitialized. A successful return does not indicate whether an
	 *         element was evicted. Element is not moved on failure.
	 * @warning Eviction advances TailIndex from the producer. Externally serialize
	 *          this call with consumers and other producers; this queue does not lock.
	 */
	FORCEINLINE SizeType PushDropOldestIfFull(ElementType&& Element, ElementType* const OutElement = nullptr) noexcept
	{
		if (QueueMax == 0)
		{
			return static_cast<SizeType>(INDEX_NONE);
		}

		const SizeType Head = HeadIndex.load(Atomic::EMemoryOrder::Relaxed);
		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);
		const SizeType NextHead = NextIndex(Head);
		if (NextHead == Tail)
		{
			ElementType* Slot = &GetElements()[Tail];

			if (OutElement)
			{
				*OutElement = MoveTempIfPossible(*Slot);
			}

			DestructItem(Slot);

			TailIndex.store(NextIndex(Tail), Atomic::EMemoryOrder::Release);
		}

		ConstructItem(&GetElements()[Head], Forward<ElementType>(Element));
		HeadIndex.store(NextHead, Atomic::EMemoryOrder::Release);
		return Head;
	}

	// Call only from the consumer after checking that the queue is not empty.
	NODISCARD FORCEINLINE ElementType Pop() noexcept
	{
		CORE_CHECK(QueueMax != 0);
		CORE_CHECK(!IsEmpty());

		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);

		ElementType* Slot = &GetElements()[Tail];
		ElementType Result(MoveTempIfPossible(*Slot));
		DestructItem(Slot);
		TailIndex.store(NextIndex(Tail), Atomic::EMemoryOrder::Release);
		return Result;
	}

	// OutElement must already exist and support move assignment. Returns false
	// if the queue is uninitialized or empty; otherwise transfers one element.
	FORCEINLINE bool TryPop(ElementType& OutElement) noexcept
	{
		if (QueueMax == 0)
		{
			return false;
		}
		if (IsEmpty())
		{
			return false;
		}

		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);

		ElementType* Slot = &GetElements()[Tail];
		OutElement = MoveTempIfPossible(*Slot); // Call move assign operator if possible.
		DestructItem(Slot);
		TailIndex.store(NextIndex(Tail), Atomic::EMemoryOrder::Release);
		return true;
	}

	// The callback sees the live front element before it is destroyed. It must
	// not retain a reference to that element after returning. This also supports
	// move-only elements that do not provide move assignment.
	template <typename ConsumerType>
	NODISCARD FORCEINLINE bool TryPopConsumer(ConsumerType&& Consumer) noexcept
	{
		if (QueueMax == 0)
		{
			return false;
		}

		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Relaxed);
		const SizeType Head = HeadIndex.load(Atomic::EMemoryOrder::Acquire);
		if (Tail == Head)
		{
			return false;
		}

		ElementType* Slot = &GetElements()[Tail];
		Invoke(Forward<ConsumerType>(Consumer), *Slot);
		DestructItem(Slot);
		TailIndex.store(NextIndex(Tail), Atomic::EMemoryOrder::Release);
		return true;
	}

	NODISCARD FORCEINLINE bool IsEmpty() const noexcept
	{
		// The consumer owns TailIndex. Acquire the producer's HeadIndex to observe
		// the element constructed before the producer published that index.
		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Relaxed);
		const SizeType Head = HeadIndex.load(Atomic::EMemoryOrder::Acquire);
		return Head == Tail;
	}

	NODISCARD FORCEINLINE bool IsFull() const noexcept
	{
		if (QueueMax == 0)
		{
			return true;
		}

		// The producer owns HeadIndex. Acquire the consumer's TailIndex to observe
		// the slot destruction completed before the consumer published that index.
		const SizeType Head = HeadIndex.load(Atomic::EMemoryOrder::Relaxed);
		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);
		return NextIndex(Head) == Tail;
	}

public:
	NODISCARD FORCEINLINE SizeType Num() const noexcept
	{
		if (QueueMax == 0)
		{
			return 0;
		}

		const SizeType Head = HeadIndex.load(Atomic::EMemoryOrder::Relaxed);
		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);
		return Head >= Tail ? Head - Tail : QueueMax - Tail + Head + 1;
	}

	NODISCARD FORCEINLINE SizeType Max() const noexcept
	{
		return QueueMax;
	}

public:
	FORCEINLINE		  ElementType& operator[](SizeType Index)	    noexcept { return At(Index); }
	FORCEINLINE const ElementType& operator[](SizeType Index) const noexcept { return At(Index); }

	FORCEINLINE ElementType& At(SizeType Index) noexcept
	{
		CORE_CHECK(QueueMax != 0);
		CORE_CHECK(0 <= Index && Index < Num());

		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);
		const SizeType PhysicalIndex = (Tail + Index) % (QueueMax + 1);
		return this->GetElements()[PhysicalIndex];
	}

	FORCEINLINE const ElementType& At(SizeType Index) const noexcept
	{
		CORE_CHECK(QueueMax != 0);
		CORE_CHECK(0 <= Index && Index < Num());

		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);
		const SizeType PhysicalIndex = (Tail + Index) % (QueueMax + 1);
		return this->GetElements()[PhysicalIndex];
	}

	NODISCARD FORCEINLINE bool TryAt(SizeType Index, ElementType& OutElement) noexcept
	{
		if (QueueMax == 0)
		{
			return false;
		}
		if (!(0 <= Index && Index < Num()))
		{
			return false;
		}

		const SizeType Tail = TailIndex.load(Atomic::EMemoryOrder::Acquire);
		const SizeType PhysicalIndex = (Tail + Index) % (QueueMax + 1);
		OutElement = this->GetElements()[PhysicalIndex];

		return true;
	}

	FORCEINLINE ElementType& PeekFront() noexcept
	{ 
		return this->At(0);
	}

	FORCEINLINE const ElementType& PeekFront() const noexcept
	{ 
		return this->At(0);
	}

public:
	FORCEINLINE bool Find(const ElementType& Element, SizeType& Index) const noexcept
	{
		Index = this->Find(Element);
		return Index != INDEX_NONE;
	}

	FORCEINLINE SizeType Find(const ElementType& Element) const noexcept
	{
		const SizeType Count = Num();
		for (SizeType Index = 0; Index < Count; ++Index)
		{
			if (Element == At(Index))
			{
				return Index;
			}
		}

		return INDEX_NONE;
	}

	template <typename Predicate>
	FORCEINLINE SizeType FindIndexByPredicate(Predicate Pred) const noexcept
	{
		const SizeType Count = Num();
		for (SizeType Index = 0; Index < Count; ++Index)
		{
			if (::Invoke(Pred, At(Index)))
			{
				return Index;
			}
		}

		return INDEX_NONE;
	}

private:
	// QueueMax is the usable capacity; the backing storage has QueueMax + 1 slots.
	NODISCARD FORCEINLINE SizeType NextIndex(SizeType Index) const noexcept
	{
		return Index == QueueMax ? 0 : Index + 1;
	}

	FORCEINLINE ElementType* GetElements() noexcept
	{
		return QueueAllocator.GetAllocation();
	}

	FORCEINLINE const ElementType* GetElements() const noexcept
	{
		return QueueAllocator.GetAllocation();
	}

public:
	NODISCARD FORCEINLINE TIterator Begin() noexcept
	{
		return TIterator(*this);
	}

	NODISCARD FORCEINLINE TIterator End() noexcept
	{
		return TIterator(*this, Num());
	}

private:
	TKernelAtomicCounter<SizeType> TailIndex;
	TKernelAtomicCounter<SizeType> HeadIndex;
	SizeType QueueMax;
	AllocatorType QueueAllocator;
};
