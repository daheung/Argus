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
		uint64 OperationId = 0;

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
	public FOperationContainer<Argus::Minifilter::FCreateOperationDescriptor, EPoolType::Paged, Argus::Minifilter::ELockType::Wait>
{
public:
	using Super = FOperationContainer<Argus::Minifilter::FCreateOperationDescriptor, EPoolType::Paged, Argus::Minifilter::ELockType::Wait>;
};
