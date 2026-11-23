#pragma once

#include "Core/Public/Policy/FilePolicyTypes.h"
#include "Kernel/Private/Minifilter/Container/OperationContainerBase.h"
#include "Kernel/Private/Minifilter/MinifilterTypes.h"
#include "Kernel/Public/Containers/KrnlSpscCircularQueue.h"
#include "Kernel/Public/Containers/KrnlString.h"
#include "Kernel/Public/Misc/KernelTypes.h"

namespace Argus::Minifilter
{
	struct FReadOperationDescriptor
	{
		/* Stream-handle context ID shared with the matching CREATE, CLEANUP, and CLOSE operations. */
		uint64 OperationId = 0;

		/* Identifier for this individual READ operation. */
		uint64 OperationReadId = 0;

		/* Pre-read observation time in UTC 100-ns intervals since 1601-01-01. */
		LARGE_INTEGER ObservedAt{};

		/* Post-read observation time; valid when bIsCompletionRead is true. */
		LARGE_INTEGER CompletionObservedAt{};

		/* Process ID of the READ requestor. */
		uint32 RequestorPid = 0;

		/* Thread ID of the READ requestor, or 0 when unavailable. */
		uint32 RequestorTid = 0;

		/* Requestor mode reported for READ. */
		EProcessorMode ProcessorMode = EProcessorMode::Unknown;

		/* Path of the file targeted by READ, when available. */
		FString<EPoolType::NonPaged> FilePathName;

		/* Number of bytes requested by READ. */
		uint32 RequestedBytes;

		/* Starting byte offset of READ; this field currently holds only 32 bits. */
		uint32 ByteOffset;

		/* Minifilter decision recorded for pre-read. */
		FPreOperationDecision PreOperationDecision;

		/* Minifilter decision recorded for post-read. */
		FPostOperationDecision PostOperationDecision;

		/* Identity of the volume containing the target file; ValidFields identifies available values. */
		Argus::Policy::FVolumeDescriptor VolumeDescriptor{};

		/* Whether the READ post-operation callback completed and supplied completion data. */
		bool bIsCompletionRead;

		/* READ completion NTSTATUS and IoStatus.Information; valid when bIsCompletionRead is true. */
		FOperationIoStatus CompletionStatus;
	};
}

using FReadOperationContainer = FOperationContainer<
	Argus::Minifilter::FReadOperationDescriptor,
	EPoolType::NonPaged,
	EContainerLockType::Spinlock>;
