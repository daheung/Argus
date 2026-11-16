#pragma once

#include "Core/Public/DeviceControl/DeviceControlTypes.h"
#include "Core/Public/Policy/FilePolicyTypes.h"

namespace Argus::DeviceControl
{
	using namespace Argus::Policy;

	struct FFltIrpOperationRP
	{
		uint32 Version;
		uint32 CurRecordCount;
		uint32 NextRecordCount;

		uint32 ProcessId; // 0 requests records from every process.

		EMinifilterPolicyOperationFlags OperationFlgs;
	};

	struct FFltIrpOperationHeader
	{
	public:
		static constexpr uint32 HeaderVersion = 1u;

	public:
		uint32 Version;
		uint64 HeaderBytes;
		uint64 TotalCount;
		uint64 TotalBytes;
		uint64 DropCount;
		uint32 NextRecordCount;
		uint64 NextRecordBytes;
	};

	enum class EFltIrpPreOperationDecision : uint8
	{
		PreOperationNone, // Unexpeced value
		PreOperationPass,
		PreOperationBlock,
		PreOperationPendingDecision,
	};

	enum class EFltIrpPostOperationDecision : uint8
	{
		PostOperationNone,
		PostOperationPass,
		PostOperationBlock,
		PostOperationPendingDecision,
	};

	struct FFltIrpPreOperationDecision
	{
		EFltIrpPreOperationDecision Decision;
		uint32 BlockStatus;
	};

	struct FFltIrpPostOperationDecision
	{
		EFltIrpPostOperationDecision Decision;
		uint32 BlockStatus;
	};

	struct FFltIrpCreateOperationRecord
	{
	public:
		static constexpr uint32 RecordAlignment = 8u;

	public:
		/* File-operation category used by policy filtering; CREATE for this record type. */
		EMinifilterPolicyOperationFlags OperationFlags;

		/* Process ID attributed to the thread that requested the file open. */
		uint32 RequestorPid;

		/* Thread ID attributed to the file-open request, when available. */
		uint32 RequestorTid;

		/* Whether the requester originated in user mode or kernel mode. */
		EProcessorMode ProcessorMode = EProcessorMode::Unknown;

		/* Time the pre-create request was observed: UTC 100-ns ticks since 1601-01-01. */
		uint64 ObservedAt{};

		/* Raw FLT_IO_PARAMETER_BLOCK::OperationFlags (SL_* flags) for this request. */
		uint8 FileOperationFlags;
		
		/* Requested FILE_SHARE_* access mask. */
		uint16 FileSharedAccess;
		
		/* File attributes supplied with the create request. */
		uint16 FileAttributes;

		/* FILE_* create disposition extracted from the high byte of Create.Options. */
		uint32 FileDisposition;
		
		/* Requested access rights from the create security context. */
		uint32 FileDesiredAccess;

		/* Minifilter policy decision for the pre-create phase. */
		FFltIrpPreOperationDecision PreOperationDecision{};
		
		/* Minifilter policy decision for the post-create phase. */
		FFltIrpPostOperationDecision PostOperationDecision{};

		/* Volume name captured before the create, with its status and payload range. */
		FStringDescriptor VolumeName;
		
		/* Opened file path captured before the create, with its status and payload range. */
		FStringDescriptor FilePathName;

		/* Volume name captured after a successful create, if available. */
		FStringDescriptor CompletionVolumeName;
		
		/* Normalized file path captured after a successful create, if available. */
		FStringDescriptor CompletionFilePathName;

		/* Reserved count of internal attempts associated with this create request. */
		uint32 InternalCallCount;
		
		/* Raw IoStatus.Information value returned by the create operation. */
		uint64 CompletionInformation;
		
		/* NTSTATUS returned by the create operation, stored as a 32-bit value. */
		uint32 CompletionStatus;

		/* Reserved for a matching IRP_MJ_CLOSE on this create's file object. */
		bool bWasIrpClosed;
		
		/* Reserved for a matching IRP_MJ_CLEANUP on this create's file object. */
		bool bWasIrpCleanedUp;

		/* FILE_SYNCHRONOUS_IO_ALERT or FILE_SYNCHRONOUS_IO_NONALERT was requested. */
		bool bIsSynchronous;
		
		/* FILE_DIRECTORY_FILE was requested; the target must be a directory. */
		bool bIsDirectoryRequired;
		
		/* FILE_NON_DIRECTORY_FILE was requested; the target must not be a directory. */
		bool bIsNonDirectoryRequired;
		
		/* FILE_NO_INTERMEDIATE_BUFFERING was requested. */
		bool bIsNonCached;
		
		/* FILE_SEQUENTIAL_ONLY was requested. */
		bool bIsSequential;
		
		/* FILE_RANDOM_ACCESS was requested. */
		bool bIsRandomAccess;
		
		/* FILE_WRITE_THROUGH was requested. */
		bool bIsWriteThrough;
		
		/* FILE_DELETE_ON_CLOSE was requested. */
		bool bIsDeleteOnClose;
		
		/* FILE_OPEN_BY_FILE_ID was requested. */
		bool bIsOpenByFileId;

		/* FILE_OPEN_REPARSE_POINT was requested. */
		bool bIsOpenReparsePoint;

		uint64 PaddingBytes;
	};
}
