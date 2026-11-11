#pragma once

#include "Kernel/Private/Minifilter/Container/OperationContainerBase.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Public/Containers/KrnlString.h"
#include "Kernel/Public/Misc/KernelTypes.h"

namespace Argus::Minifilter
{
	enum class ECreatePreOperationDecision : uint8
	{
		PreOperationPass,
		PreOperationBlock,
		PreOperationPending,
	};

	enum class ECreatePostOperationDecision : uint8
	{
		PostOperationPass,
		PostOperationBlock,
		PostOperationMoreProcessingRequired,
	};

	struct FCreatePreOperationDecision
	{
		ECreatePreOperationDecision Decision = ECreatePreOperationDecision::PreOperationPass;
		NTSTATUS BlockStatus = STATUS_ACCESS_DENIED;
	};

	struct FCreatePostOperationDecision
	{
		ECreatePostOperationDecision Decision = ECreatePostOperationDecision::PostOperationPass;
		NTSTATUS BlockStatus = STATUS_ACCESS_DENIED;
	};

	enum class EOperationStringStatus : uint8
	{
		NotApplicable,
		Captured,
		Unavailable,
		AllocationFailed,
	};

	struct FCreateOperationDescriptor
	{
		uint32 RequestorPid = 0;
		uint32 RequestorTid = 0;
		EProcessorMode ProcessorMode = EProcessorMode::Unknown;

		LARGE_INTEGER ObservedAt{};

		uint8 OperationFlags = 0;
		uint16 SharedAccess = 0;
		uint16 Attribute = 0;

		uint32 Disposition = 0;
		ACCESS_MASK DesiredAccess = 0;

		//EFilterMode FilterMode = EFilterMode::Pass;
		FCreatePreOperationDecision PreOperationDecision{};
		FCreatePostOperationDecision PostOperationDecision{};

		EOperationStringStatus VolumeNameStatus = EOperationStringStatus::NotApplicable;
		EOperationStringStatus FilePathNameStatus = EOperationStringStatus::NotApplicable;

		FString<> VolumeName{};
		FString<> FilePathName{};

		EOperationStringStatus CompletionVolumeNameStatus = EOperationStringStatus::NotApplicable;
		EOperationStringStatus CompletionFilePathNameStatus = EOperationStringStatus::NotApplicable;

		FString<> CompletionVolumeName{};
		FString<> CompletionFilePathName{};

		uint32 InternalAttemptedCount = 0;
		uint64 CompletionInformation = 0;
		NTSTATUS CompletionStatus = STATUS_UNSUCCESSFUL;

		bool bSynchronous = false;
		bool bDirectoryRequired = false;
		bool bNonDirectoryRequired = false;
		bool bNonCached = false;
		bool bSequential = false;
		bool bRandomAccess = false;
		bool bWriteThrough = false;
		bool bDeleteOnClose = false;
		bool bOpenByFileId = false;
		bool bOpenReparsePoint = false;
	};
}

class FCreateOperationContainer :
	public IOperationContainerBase<Argus::Minifilter::FCreateOperationDescriptor, Argus::Minifilter::ELockType::Wait>
{
public:
	typedef IOperationContainerBase<Argus::Minifilter::FCreateOperationDescriptor, Argus::Minifilter::ELockType::Wait> Super;

	typedef Argus::Minifilter::FCreateOperationDescriptor DescriptorType;
	typedef TOperationContainerLock<Argus::Minifilter::ELockType::Wait> LockType;
	typedef TSpscCircularQueue<EPoolType::Paged, DescriptorType> TQueue;

public:
	FCreateOperationContainer()
		: Super()
		, Container()
	{}

public:
	NODISCARD bool Initialize() noexcept
	{
		FWriteGuard Guard(this->Lock);
		if (bInitialized)
		{
			return true;
		}

		bInitialized = Container.Initialize(MaxEvents) == EContainerStatus::Success;
		return bInitialized;
	}

	FORCEINLINE bool Push(DescriptorType&& Descriptor)
	{
		FWriteGuard Guard(this->Lock);
		if (!bInitialized || Container.Push(MoveTempIfPossible(Descriptor)) == INDEX_NONE)
		{
			++DroppedCount;
			return false;
		}
		return true;
	}

	NODISCARD bool TryPop(DescriptorType& OutDescriptor) noexcept
	{
		FWriteGuard Guard(this->Lock);
		return bInitialized && Container.TryPopConsumer([&OutDescriptor](DescriptorType& QueuedDescriptor) noexcept
		{
			DestructItem(&OutDescriptor);
			ConstructItem(&OutDescriptor, MoveTemp(QueuedDescriptor));
		});
	}

	NODISCARD uint64 GetDroppedCount() noexcept
	{
		FReadGuard Guard(this->Lock);
		return DroppedCount;
	}

	static constexpr int32 MaxEvents = 1024;

private:
	TQueue Container;
	uint64 DroppedCount = 0;
	bool bInitialized = false;
};
