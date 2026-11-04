#pragma once


#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlPoolType.h"
#include "Core/Public/Misc/Types.h"
#include "Core/Public/Templates/TypeTraits.h"

#if defined(PLATFORM_WDM) || defined(PLATFORM_KMDF)

namespace Argus::Allocator
{
    template <typename SizeType>
    NODISCARD FORCEINLINE constexpr SizeType CalculateGrowCapacity(
        SizeType CurrentMax, SizeType Required, SizeType MaxSize) noexcept
    {
        static_assert(!TIsSigned<SizeType>::Value, "Container growth requires an unsigned size type");

        if (Required <= CurrentMax)
        {
            return CurrentMax;
        }

        const SizeType Growth = CurrentMax / 6 > 0 
            ? static_cast<SizeType>(CurrentMax / 6)
            : static_cast<SizeType>(1);
        const SizeType Candidate = CurrentMax > MaxSize - Growth
            ? MaxSize : CurrentMax + Growth;
        return Candidate > Required ? Candidate : Required;
    }

    struct FPoolBlock
    {
        void* PoolPointer = nullptr; // Pointer to pass to ExFreePool.
        void* Allocation = nullptr;  // Aligned address visible to the container.
        size_t AllocationSize = 0;   // Usable bytes, not raw pool allocation size.
        size_t Alignment = 0;
    };

    inline bool IsPowerOfTwo(size_t Value) noexcept
    {
        return Value != 0 && (Value & (Value - 1)) == 0;
    }

    inline void FreePoolBlock(FPoolBlock& Block) noexcept
    {
        if (Block.PoolPointer != nullptr)
        {
            ExFreePool(Block.PoolPointer);
        }

        Block = {};
    }

    template <EPoolType Type>
    FORCEINLINE bool AllocatePoolBlock(size_t Bytes, size_t Alignment, ULONG Tag, FPoolBlock& OutBlock) noexcept
    {
        const bool bInvalidAlignment = !IsPowerOfTwo(Alignment);
        if (Bytes == 0 || bInvalidAlignment || Tag == 0 || OutBlock.PoolPointer != nullptr)
        {
            return false;
        }

        const size_t Padding = Alignment - 1;
        const size_t MaxSize = static_cast<size_t>(-1);
        if (Bytes > MaxSize - Padding)
        {
            return false;
        }

        POOL_EXTENDED_PARAMETER Parameter{};
        Parameter.Type = PoolExtendedParameterPriority;
        Parameter.Priority = NormalPoolPriority;

        constexpr POOL_FLAGS Flags = (Type == EPoolType::Paged)
            ? POOL_FLAG_PAGED
            : POOL_FLAG_NON_PAGED;

        void* PoolPointer = ExAllocatePool3(Flags, Bytes + Padding, Tag, &Parameter, 1);
        if (PoolPointer == nullptr)
        {
            return false;
        }

        const size_t Address = reinterpret_cast<size_t>(PoolPointer);
        const size_t Mask = static_cast<size_t>(Padding);
        const size_t MaxAddress = static_cast<size_t>(INDEX_NONE);
        if (Address > MaxAddress - Mask)
        {
            ExFreePool(PoolPointer);
            return false;
        }

        const size_t AlignedAddress = (Address + Mask) & ~Mask;
        OutBlock.PoolPointer = PoolPointer;
        OutBlock.Allocation = reinterpret_cast<void*>(AlignedAddress);
        OutBlock.AllocationSize = Bytes;
        OutBlock.Alignment = Alignment;
        return true;
    }
}

template <typename ToAllocatorType, typename FromAllocatorType>
struct TCanAccessibleToOtherAllocator
{
    static constexpr bool Value = TCanAccessibleToOtherAllocator<ToAllocatorType, FromAllocatorType>::Impl();

private:
    constexpr static bool Impl() noexcept
    {
        constexpr EPoolType ToPoolType = ToAllocatorType::PoolType;
        constexpr EPoolType FromPoolType = FromAllocatorType::PoolType;

        if constexpr (ToPoolType == EPoolType::NonPaged && FromPoolType == EPoolType::Paged)
        {
            return false;
        }

        return true;
    }
};


template <typename ToAllocatorType, typename FromAllcoatorType>
struct TCanTransferAllocationOwnership
{
    static constexpr bool Value = false;
};

// Owns one dynamically allocated, contiguous RAW MEMORY block.
// The allocator itself may move, but it must never be copied (double-free).
template <typename InSizeType, EPoolType Type>
class TKrnlLinearAllocator
{
public:
    using SizeType = typename TMakeSigned<InSizeType>::Type;
    using USizeType = typename TMakeUnsigned<SizeType>::Type;

    enum : uint8 { ForAnyElementType = true, NeedsElementType = false };
    static constexpr EPoolType PoolType = Type;

public:
    TKrnlLinearAllocator() noexcept
        : PoolTag('mAeM')
        , Block()
    {
    }

    explicit TKrnlLinearAllocator(ULONG InPoolTag) noexcept
        : PoolTag(InPoolTag)
        , Block()
    {
    }

    ~TKrnlLinearAllocator() noexcept
    {
        Deallocate();
    }

    TKrnlLinearAllocator(const TKrnlLinearAllocator&) = delete;
    TKrnlLinearAllocator& operator=(const TKrnlLinearAllocator&) = delete;

    TKrnlLinearAllocator(TKrnlLinearAllocator&& Other) noexcept
        : PoolTag(Other.PoolTag)
        , Block(Other.Block)
    {
        Other.Block = {};
    }

    TKrnlLinearAllocator& operator=(TKrnlLinearAllocator&& Other) noexcept
    {
        if (this != &Other)
        {
            // All live elements, if any, must already have been destroyed
            // by the container before its current raw storage is released.
            Deallocate();

            PoolTag = Other.PoolTag;
            Block = Other.Block;
            Other.Block = {};
        }
        return *this;
    }

    void* Allocate(SizeType ElementCount, SizeType ElementSize, SizeType Alignment) noexcept
    {
        if (HasAllocation())
        {
            NT_ASSERT(nullptr);
            return nullptr; // Never overwrite an owned allocation; Use Reallocate instead.
        }

        size_t Bytes = 0;
        size_t RequestedAlignment = 0;
        if (!TryGetByteCount(ElementCount, ElementSize, Bytes) ||
            !TryGetAlignment(Alignment, RequestedAlignment))
        {
            return nullptr;
        }

        Argus::Allocator::FPoolBlock NewBlock{};
        if (!Argus::Allocator::AllocatePoolBlock<Type>(Bytes, RequestedAlignment, PoolTag, NewBlock))
        {
            return nullptr;
        }

        Block = NewBlock;
        return Block.Allocation;
    }

    void* AllocateWithAlignment(SizeType ElementCount, SizeType ElementSize, SizeType Alignment) noexcept
    {
        return Allocate(ElementCount, ElementSize, Alignment);
    }

    void Deallocate() noexcept
    {
        Argus::Allocator::FreePoolBlock(Block);
    }

    // Byte-wise relocation only! See the contract at the beginning of file.
    // On failure, the original allocation and its contents remain intact.
    // NewElCount == 0 releases the allocation (after the container destroys elements).
    void* Reallocate(SizeType OldElCount, SizeType NewElCount, SizeType ElementSize, SizeType Alignment) noexcept
    {
        size_t OldBytes = 0;
        size_t NewBytes = 0;
        size_t RequestedAlignment = 0;
        if (!TryGetByteCount(OldElCount, ElementSize, OldBytes) ||
            !TryGetByteCount(NewElCount, ElementSize, NewBytes) ||
            !TryGetAlignment(Alignment, RequestedAlignment))
        {
            return nullptr;
        }

        if (OldBytes > Block.AllocationSize || (OldBytes != 0 && !HasAllocation()))
        {
            return nullptr;
        }

        if (NewBytes == 0)
        {
            Deallocate();
            return nullptr;
        }

        if (HasAllocation() && Block.AllocationSize == NewBytes &&
            Block.Alignment >= RequestedAlignment)
        {
            return Block.Allocation;
        }

        Argus::Allocator::FPoolBlock NewBlock{};
        if (!Argus::Allocator::AllocatePoolBlock<Type>(NewBytes, RequestedAlignment, PoolTag, NewBlock))
        {
            return nullptr;
        }

        const size_t BytesToCopy = (OldBytes < NewBytes) ? OldBytes : NewBytes;
        if (BytesToCopy != 0)
        {
            RtlCopyMemory(NewBlock.Allocation, Block.Allocation, BytesToCopy);
        }

        Argus::Allocator::FreePoolBlock(Block);
        Block = NewBlock;
        return Block.Allocation;
    }

    /**
     * Transfers ownership of Block itself; no element is moved or constructed.
     * 
     * Bitwise relocatability is not enough: both containers must have the exact
     * same ElementType because the existing objects keep their identities and
     * lifetimes. 
     * 
     * This raw allocator cannot enforce that; the typed allocator or
     * container must do so. Different element types require element construction
     * in new storage. NonPaged storage may be transferred to a Paged allocator,
     * but Paged storage cannot satisfy a NonPaged allocator's access contract.
     */
    template <typename OtherSizeType, EPoolType OtherType>
    bool MoveToEmptyFromOtherAllocator(TKrnlLinearAllocator<OtherSizeType, OtherType>& Other) noexcept
    {
        if (HasAllocation() || PoolTag != Other.PoolTag ||
            !CanRepresent(Other.Block.AllocationSize) ||
            !CanRepresent(Other.Block.Alignment))
        {
            return false;
        }

        using OtherAllocatorType = TKrnlLinearAllocator<OtherSizeType, OtherType>;
        if constexpr (!TCanAccessibleToOtherAllocator<TKrnlLinearAllocator, OtherAllocatorType>::Value)
        {
            return false;
        }

        Block = Other.Block;
        Other.Block = {};
        return true;
    }

    // Raw byte-copy. Only use on unconstructed or byte-copy-safe storage.
    template <typename OtherSizeType, EPoolType OtherType>
    bool CopyToEmptyFromOtherAllocator(const TKrnlLinearAllocator<OtherSizeType, OtherType>& Other) noexcept
    {
        if (HasAllocation() || !CanRepresent(Other.Block.AllocationSize) ||
            !CanRepresent(Other.Block.Alignment))
        {
            return false;
        }
        if (!Other.HasAllocation())
        {
            return true;
        }

        Argus::Allocator::FPoolBlock NewBlock{};
        if (!Argus::Allocator::AllocatePoolBlock<Type>(Other.Block.AllocationSize, Other.Block.Alignment, PoolTag, NewBlock))
        {
            return false;
        }

        RtlCopyMemory(NewBlock.Allocation, Other.Block.Allocation, Other.Block.AllocationSize);
        Block = NewBlock;
        return true;
    }

public:
    NODISCARD FORCEINLINE size_t GetAllocatedSize(SizeType NumElements, size_t NumBytesPerElement) const noexcept
    {
        const size_t AllocationSize = this->Block.AllocationSize;
        CORE_CHECK(NumElements * NumBytesPerElement == AllocationSize);
        return AllocationSize;
    }

    NODISCARD FORCEINLINE bool HasAllocation() const noexcept 
    { 
        return Block.Allocation != nullptr; 
    }

    NODISCARD FORCEINLINE       void* GetAllocation()       noexcept { return Block.Allocation; }
    NODISCARD FORCEINLINE const void* GetAllocation() const noexcept { return Block.Allocation; }

    NODISCARD FORCEINLINE size_t GetAllocationSize() const noexcept 
    { 
        return Block.AllocationSize; 
    }

    NODISCARD FORCEINLINE size_t GetAllocationAlignment() const noexcept 
    { 
        return Block.Alignment; 
    }

    NODISCARD FORCEINLINE uint32 GetPoolTag() const noexcept 
    { 
        return PoolTag; 
    }

private:
    template <typename, EPoolType>
    friend class TKrnlLinearAllocator;

    static FORCEINLINE bool CanRepresent(size_t Value) noexcept
    {
        if (Value == 0)
        {
            return true;
        }

        const SizeType Converted = static_cast<SizeType>(Value);
        return Converted > 0 && static_cast<size_t>(Converted) == Value;
    }

    static bool TryGetByteCount(SizeType Count, SizeType ElementSize, size_t& OutBytes) noexcept
    {
        if (ElementSize <= 0 || Count < 0)
        {
            return false;
        }
        if (Count == 0)
        {
            OutBytes = 0;
            return true;
        }

        const size_t CountValue = static_cast<size_t>(Count);
        const size_t SizeValue  = static_cast<size_t>(ElementSize);
        if (static_cast<SizeType>(CountValue) != Count ||
            static_cast<SizeType>(SizeValue ) != ElementSize ||
            CountValue > static_cast<size_t>(INDEX_NONE) / SizeValue)
        {
            return false;
        }

        const size_t Bytes = CountValue * SizeValue;
        if (!CanRepresent(Bytes))
        {
            return false;
        }

        OutBytes = Bytes;
        return true;
    }

    static FORCEINLINE bool TryGetAlignment(SizeType Alignment, size_t& OutAlignment) noexcept
    {
        if (Alignment <= 0)
        {
            return false;
        }

        const size_t Value = static_cast<size_t>(Alignment);
        if (!CanRepresent(Value) || !Argus::Allocator::IsPowerOfTwo(Value))
        {
            return false;
        }

        OutAlignment = Value;
        return true;
    }

    ULONG PoolTag;
    Argus::Allocator::FPoolBlock Block;
};

// Type-aware pointer interface. Element lifetime still belongs to the container.
template <typename InElementType, typename InSizeType, EPoolType Type>
class TKrnlElLinearAllocator : public TKrnlLinearAllocator<InSizeType, Type>
{
private:
    using Super = TKrnlLinearAllocator<InSizeType, Type>;

public:
    using typename TKrnlLinearAllocator<InSizeType, Type>::SizeType;
    using typename TKrnlLinearAllocator<InSizeType, Type>::USizeType;

    using ElementType = InElementType;
    enum : uint8 { ForAnyElementType = false, NeedsElementType = true };

public:
    TKrnlElLinearAllocator() noexcept = default;

    explicit TKrnlElLinearAllocator(ULONG InPoolTag) noexcept
        : Super(InPoolTag)
    {
    }

    TKrnlElLinearAllocator(const TKrnlElLinearAllocator&) = delete;
    TKrnlElLinearAllocator& operator=(const TKrnlElLinearAllocator&) = delete;

    TKrnlElLinearAllocator(TKrnlElLinearAllocator&&) noexcept = default;
    TKrnlElLinearAllocator& operator=(TKrnlElLinearAllocator&&) noexcept = default;

    ElementType* Allocate(SizeType ElementCount,
        SizeType ElementSize = static_cast<SizeType>(sizeof(ElementType)),
        SizeType Alignment   = static_cast<SizeType>(alignof(ElementType))) noexcept
    {
        if (ElementSize != sizeof(ElementType) || !IsValidAlignment(Alignment))
        {
            return nullptr;
        }
        return static_cast<ElementType*>(Super::Allocate(
            ElementCount, ElementSize, EffectiveAlignment(Alignment)));
    }

    ElementType* Reallocate(SizeType OldElCount, SizeType NewElCount,
        SizeType ElementSize = static_cast<SizeType>(sizeof(ElementType)),
        SizeType Alignment   = static_cast<SizeType>(alignof(ElementType))) noexcept
    {
        if (ElementSize != sizeof(ElementType) || !IsValidAlignment(Alignment))
        {
            return nullptr;
        }
        return static_cast<ElementType*>(Super::Reallocate(
            OldElCount, NewElCount, ElementSize, EffectiveAlignment(Alignment)));
    }

    // The parameter type enforces the same ElementType for block ownership transfer.
    template <typename OtherSizeType, EPoolType OtherType>
    bool MoveToEmptyFromOtherAllocator(
        TKrnlElLinearAllocator<ElementType, OtherSizeType, OtherType>& Other) noexcept
    {
        return Super::MoveToEmptyFromOtherAllocator(static_cast<TKrnlLinearAllocator<OtherSizeType, OtherType>&>(Other));
    }

    template <typename OtherSizeType, EPoolType OtherType>
    bool CopyToEmptyFromOtherAllocator(
        const TKrnlElLinearAllocator<ElementType, OtherSizeType, OtherType>& Other) noexcept
    {
        return Super::CopyToEmptyFromOtherAllocator(static_cast<const TKrnlLinearAllocator<OtherSizeType, OtherType>&>(Other));
    }

    ElementType* GetAllocation() noexcept
    {
        return static_cast<ElementType*>(Super::GetAllocation());
    }

    const ElementType* GetAllocation() const noexcept
    {
        return static_cast<const ElementType*>(Super::GetAllocation());
    }

private:
    static bool IsValidAlignment(SizeType Alignment) noexcept
    {
        return Alignment > 0 &&
            Argus::Allocator::IsPowerOfTwo(static_cast<SIZE_T>(Alignment));
    }

    static SizeType EffectiveAlignment(SizeType Alignment) noexcept
    {
        const SizeType ElementAlignment = static_cast<SizeType>(alignof(ElementType));
        return Alignment > ElementAlignment ? Alignment : ElementAlignment;
    }
};

// Fixed-capacity RAW STORAGE; no pool allocation and no element construction.
// Its buffer is part of the allocator object and cannot be 'stolen' or moved
// by copying the bytes. The CONTAINER handles element-by-element relocation.
template <typename InElementType, typename InSizeType, int32 InNumElements>
class TKnrlElFixedAllocator
{
public:
    using ElementType = InElementType;
    using SizeType = InSizeType;
    using USizeType = SIZE_T;

    static_assert(InNumElements > 0, "Fixed allocator capacity must be positive.");
    static constexpr int32 NumElements = InNumElements;
    enum : uint8 { ForAnyElementType = false, NeedsElementType = true };

    TKnrlElFixedAllocator() noexcept = default;
    ~TKnrlElFixedAllocator() noexcept = default; // Destroys NO elements.

    TKnrlElFixedAllocator(const TKnrlElFixedAllocator&) = delete;
    TKnrlElFixedAllocator& operator=(const TKnrlElFixedAllocator&) = delete;
    TKnrlElFixedAllocator(TKnrlElFixedAllocator&&) = delete;
    TKnrlElFixedAllocator& operator=(TKnrlElFixedAllocator&&) = delete;

    ElementType* Allocate(SizeType ElementCount,
        SizeType ElementSize = static_cast<SizeType>(sizeof(ElementType)),
        SizeType Alignment = static_cast<SizeType>(alignof(ElementType))) noexcept
    {
        if (!IsValidRequest(ElementCount, ElementSize, Alignment))
        {
            return nullptr;
        }
        return GetAllocation();
    }

    ElementType* Reallocate(SizeType OldElCount, SizeType NewElCount,
        SizeType ElementSize = static_cast<SizeType>(sizeof(ElementType)),
        SizeType Alignment = static_cast<SizeType>(alignof(ElementType))) noexcept
    {
        if (!IsValidRequest(OldElCount, ElementSize, Alignment) ||
            !IsValidRequest(NewElCount, ElementSize, Alignment))
        {
            return nullptr;
        }
        return GetAllocation(); // No physical reallocation is necessary.
    }

    void Deallocate() noexcept {} // Storage belongs to *this*, not the pool.
    bool HasAllocation() const noexcept { return true; }

    ElementType* GetAllocation() noexcept
    {
        return reinterpret_cast<ElementType*>(Storage);
    }
    const ElementType* GetAllocation() const noexcept
    {
        return reinterpret_cast<const ElementType*>(Storage);
    }
    constexpr SIZE_T GetAllocationSize() const noexcept
    {
        return sizeof(ElementType) * static_cast<SIZE_T>(NumElements);
    }
    constexpr SIZE_T GetAllocationAlignment() const noexcept
    {
        return alignof(ElementType);
    }

    // Fixed storage is not transferable; container relocation is required.
    template <typename OtherAllocator>
    bool MoveToEmptyFromOtherAllocator(OtherAllocator&) = delete;
    template <typename OtherAllocator>
    bool CopyToEmptyFromOtherAllocator(const OtherAllocator&) = delete;

private:
    static bool IsValidRequest(SizeType ElementCount, SizeType ElementSize,
        SizeType Alignment) noexcept
    {
        if (ElementCount < 0 || ElementSize != sizeof(ElementType) ||
            Alignment <= 0 ||
            !Argus::Allocator::IsPowerOfTwo(static_cast<SIZE_T>(Alignment)))
        {
            return false;
        }
        return static_cast<SIZE_T>(ElementCount) <= static_cast<SIZE_T>(NumElements) &&
            static_cast<SIZE_T>(Alignment) <= alignof(ElementType);
    }

    alignas(ElementType) unsigned char Storage[sizeof(ElementType) * static_cast<SIZE_T>(NumElements)];
};

template <typename ElementType, typename SizeType = int32, EPoolType PageFlag = EPoolType::Paged>
using TKrnlDefaultAllocator = TKrnlElLinearAllocator<ElementType, SizeType, PageFlag>;

template <typename ElementType, int32 NumElements, typename SizeType = int32>
using TKrnlFixedAllocator = TKnrlElFixedAllocator<ElementType, SizeType, NumElements>;

// Correctly spelled convenience alias; old TKnrlElFixedAllocator is retained.
template <typename ElementType, typename SizeType, int32 NumElements>
using TKrnlElFixedAllocator = TKnrlElFixedAllocator<ElementType, SizeType, NumElements>;

namespace Argus::Allocator::TypeTraits
{
    /**
     * 서로 다른 Allocator 간의 Swap 가능 여부와 처리 방법에 대한 설계 메모.
     *
     * 서로 다른 Allocator들이 추가 할당이나 Element 재배치 없이,
     * 실패하지 않고 allocation의 소유권만 교환할 수 있는 조건은 다음과 같음.
     *
     *  1. 양쪽 Allocator의 ElementType이 동일할 것.
     *  2. 양쪽 ElementType이 trivially relocatable할 것. (x)
     *   - Allocator의 Swap은 block을 통쨰로 Swap 함으로써 Element-wise relocatable을 수행하지 않음
     *  3. Allocator가 allocation의 소유권을 원소 이동 없이 이전할 수 있을 것.
     *  4. Pool Type이 동일하거나, 서로 호환되는 메모리 관리 정책을 가질 것.
     *  5. Swap 이후에도 각 allocation을 올바르게 해제할 수 있을 것.
     *  6. Allocator의 Swap이 실패하지 않을 것.
     *
     * 어느 한쪽 Allocator에 ElementType이 정의되지 않았다면 false 반환.
     *
     * 해당 Type Trait의 목적은
     * 'Element별 메모리 이동 없이 allocation의 소유권만 안전하게 교환할 수 있는가?'
     * 를 컴파일 시간에 판단하는 것임.
     *
     * 해당 Type Trait이 보장하려는 것은 다음과 같음.
     *
     *  1. 양쪽 Allocator가 Swap을 수행하였을 때 함수가 실패하지 않음.
     *   - 메모리 시작 주소, Tag 및 필요한 소유권 상태만 교환하기 때문에 실패할 수 있는 경우의 수가 존재하지 않음.
     *     따라서, 추가적인 메모리 할당이나 Element 생성이 발생하지 않음.
     *   - 단순 포인터 교환만으로 충분한지는 Allocator의 구현에 따라 달라짐.
     *
     *  2. Swap 이후 Element별 메모리 재배치가 필요하지 않음.
     *   - 기존 Element의 메모리 주소와 객체 생명주기가 그대로 유지됨.
     *   - 따라서 Element가 Trivially Relocatable일 필요는 없음.
     *
     *
     * 또한, Allocator 자체의 특성을 반영할 필요가 있음.
     *
     * FixedAllocator처럼 메모리를 Allocator 내부에 직접 보유하는 경우,
     * 단순한 포인터 교환으로는 allocation의 소유권을 이전할 수 없음.
     *
     * 이러한 Allocator는 별도의 Trait 특수화로 false 처리하거나,
     * Allocator 간 allocation 소유권 이전 가능 여부를 검사하는
     * 별도의 Type Trait을 도입할 필요가 있을 수 있음.
     *
     *
     * Allocator를 사용하는 Container가 Swap을 수행할 때
     * 발생할 수 있는 경우는 다음과 같음.
     *
     *  1. allocation의 소유권만 교환하는 경우.
     *   - 메모리 시작 주소 및 관련 소유권 상태만 교환함.
     *   - Element의 재배치나 생성 및 소멸이 발생하지 않음.
     *   - 추가 할당 X
     *
     *  2. Element마다 Bitwise 수준의 메모리 재배치를 수행하는 경우.
     *   - 기존 allocation의 소유권을 직접 교환할 수 없음.
     *   - Element별 메모리 이동이 필요함.
     *   - 해당 Element가 Bitwise Relocation을 허용해야 함. 
     *     모든 Allocator는 Bitwise relocation이 가능하다는 전제로 작성 되기 때문에
     *     이를 보장하지 않는 경우, 사용자가 따로 처리해줘야 함.
     *   - 따라서, 단순 메모리 복사가 객체 생명주기를 올바르게 처리하는지도 보장해야 함.
     *
     *  3. Element마다 생성 및 소멸 연산을 수행해야 하는 경우.
     *   - Bitwise Relocation만으로 처리할 수 없음.
     *   - Placement New 등을 이용한 Move/Copy Construction이 필요함.
     *   - 기존 Element의 소멸 및 객체 생명주기를 별도로 관리해야 함.
     *
     *  4. 교환이 불가능한 경우.
     *   - Allocator 또는 Element의 제약으로 인해 Swap을 수행할 수 없음.
     *
     *
     * 위의 2번과 3번을 구분하는 것은 Allocator 자체보다는
     * Allocator를 사용하는 Container 및 Element 처리 코드의 책임임.
     *
     * 따라서 Allocator의 allocation 소유권 교환과
     * Element의 재배치 가능 여부는 별도로 판단해야 함.
     *
     *
     * Swap이 불가능하거나 제한되는 조건은 다음과 같음.
     *
     *  1. 서로 다른 Pool Type 또는 메모리 관리 정책을 사용하는 경우.
     *   - Paged / NonPaged Pool의 차이를 고려해야 함.
     *   - Pool Type이 다르다는 이유만으로 소유권 교환이 항상 불가능한 것은 아님.
     *   - 단, Container가 요구하는 메모리 접근 조건을 만족하지 못할 수 있음.
     *   - 초기 구현에서는 서로 다른 Pool Type 간의 직접 교환을 제한하는 방향 고려.
     *
     *  2. Allocator 자체가 allocation 소유권 이전을 지원하지 않는 경우.
     *   - FixedAllocator처럼 내부 Storage에 종속되는 경우.
     *   - allocation의 해제 책임을 안전하게 이전할 수 없는 경우.
     *
     *  3. IRQL 제약으로 인해 필요한 작업을 수행할 수 없는 경우.
     *   - 높은 IRQL에서는 메모리 할당, 해제 또는 접근에 제약이 존재함.
     *   - 특히 Paged Memory에 접근하는 연산을 주의해야 함.
     *   - 향후 DMA 및 DMA Remapping 관련 메모리 관리에서도
     *     별도의 제약 사항을 고려해야 할 것으로 보임.
     *   - IRQL은 실행 시점에 따라 달라질 수 있으므로
     *     모든 조건을 컴파일 시간에 검사할 수 있는 것은 아님.
     *   - 해당 조건의 구체적인 처리 방법은 추후 도입 예정.
     *
     *
     * 이러한 문제를 가능한 한 컴파일 시간에 검사하여
     * 잘못된 Swap 경로가 선택되는 것을 사전에 차단하는 것이 목적임.
     *
     * 단, 실행 시점의 IRQL이나 동적으로 결정되는 메모리 상태 등은
     * 별도의 Runtime Validation이 필요할 수 있음.
     *
     *
     * 또한, Container가 서로 다른 Allocator를 사용하고 있을 때,
     * 단순히 Allocator의 memory swap만 호출해서는 안 되는 경우가 있음.
     *
     * allocation 소유권의 직접 교환이 불가능하다면
     * RelocateItemsFrom 등을 이용하여 Element를 재배치해야 할 수도 있음.
     *
     * 특히 Element가 Trivially Relocatable하지 않은 경우에는
     * 단순 메모리 복사만으로 처리할 수 없으며,
     * 객체 생성, 소멸 및 소유권 관리에 문제가 발생하지 않도록 해야 함.
     *
     * 단, allocation의 소유권 자체를 안전하게 교환할 수 있다면
     * Element가 Trivially Relocatable하지 않아도
     * 개별 Element의 재배치는 필요하지 않음.
     *
     * 최종적으로는 다음 두 가지를 독립적으로 검사하는 방향을 고려.
     *
     *  1. Allocator 간 allocation의 소유권을 직접 교환할 수 있는가?
     *  2. 직접 교환할 수 없다면 Element를 어떤 방식으로 재배치할 수 있는가?
     */

    /**
     * @brief Indicates whether two allocator types can exchange allocation ownership without allocating.
     * @tparam ToAllocatorType Allocator type of the first array.
     * @tparam FromAllocatorType Allocator type of the second array.
     *
     * Value is true only for matching linear allocator specializations: the size
     * type and pool type must match, and typed allocators must also have the same
     * element type. Fixed and other allocator types return false. This trait
     * checks allocator types only; TKrnlArray::Swap separately checks array
     * element and pool types.
     */
    template <typename AllocatorTypeA, typename AllocatorTypeB>
    struct TCanSwapAllocationBlocks
    {
        static constexpr bool Value = false;
    };
    
    template <typename ElementTypeA, typename SizeTypeA, EPoolType PoolTypeA,
        typename ElementTypeB, typename SizeTypeB, EPoolType PoolTypeB>
    struct TCanSwapAllocationBlocks<
        TKrnlElLinearAllocator<ElementTypeA, SizeTypeA, PoolTypeA>,
        TKrnlElLinearAllocator<ElementTypeB, SizeTypeB, PoolTypeB>>
    {
        static constexpr bool Value = 
            TAnd<
                TIsSame<ElementTypeA, ElementTypeB>,
                TIsSame<SizeTypeA, SizeTypeB>
            >::Value &&
            PoolTypeA == PoolTypeB;
    };
}

#endif
