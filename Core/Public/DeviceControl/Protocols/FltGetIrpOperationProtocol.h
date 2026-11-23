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
		/* Operation category; always Create for this record. */
		EMinifilterPolicyOperationFlags OperationFlags;

		/* Process ID of the CREATE requestor. */
		uint32 RequestorPid;

		/* Thread ID of the CREATE requestor, or 0 when unavailable. */
		uint32 RequestorTid;

		/* Requestor mode reported for CREATE. */
		EProcessorMode ProcessorMode = EProcessorMode::Unknown;

		/* Pre-create observation time: UTC 100-ns intervals since 1601-01-01. */
		uint64 ObservedAt{};

		/* Raw FLT_IO_PARAMETER_BLOCK::OperationFlags (SL_* flags) for CREATE. */
		uint8 FileOperationFlags;
		
		/* Requested FILE_SHARE_* flags. */
		uint16 FileSharedAccess;
		
		/* File attributes supplied with CREATE. */
		uint16 FileAttributes;

		/* FILE_* disposition from the high byte of Create.Options. */
		uint32 FileDisposition;
		
		/* DesiredAccess from the CREATE security context, or 0 when absent. */
		uint32 FileDesiredAccess;

		/* Minifilter decision recorded for pre-create. */
		FFltIrpPreOperationDecision PreOperationDecision{};
		
		/* Minifilter decision recorded for post-create. */
		FFltIrpPostOperationDecision PostOperationDecision{};

		/* Volume name captured before CREATE; status and payload location. */
		EStringStatus VolumeNameStatus = EStringStatus::NotApplicable;
		FStringDescriptor VolumeName;
		
		/* Opened file path captured before CREATE; status and payload location. */
		EStringStatus FilePathNameStatus = EStringStatus::NotApplicable;
		FStringDescriptor FilePathName;

		/* Volume name captured during post-create, if available. */
		EStringStatus CompletionVolumeNameStatus = EStringStatus::NotApplicable;
		FStringDescriptor CompletionVolumeName;
		
		/* Normalized file path captured during post-create, if available. */
		EStringStatus CompletionFilePathNameStatus = EStringStatus::NotApplicable;
		FStringDescriptor CompletionFilePathName;

		/* Reserved for internal CREATE attempts; currently always 0. */
		uint32 InternalCallCount;
		
		/* Raw IoStatus.Information from CREATE completion. */
		uint64 CompletionInformation;
		
		/* CREATE completion NTSTATUS stored as a 32-bit value. */
		uint32 CompletionStatus;

		/* Matching CLOSE pre-operation time in UTC 100-ns intervals; valid when bWasIrpClosed is true. */
		int64 IrpClosedAt;

		/* Whether a matching IRP_MJ_CLOSE was collected before CREATE retrieval. */
		bool bWasIrpClosed;

		/* Matching CLEANUP post-operation time in UTC 100-ns intervals; valid when bWasIrpCleanedUp is true. */
		int64 IrpCleanupCompletionAt;

		/* Raw IoStatus.Information from matching CLEANUP; valid when bWasIrpCleanedUp is true. */
		uint64 IrpCleanupCompletionInformation;

		/* Matching CLEANUP NTSTATUS as a 32-bit value; valid when bWasIrpCleanedUp is true. */
		uint32 IrpCleanupCompletionStatus;
		
		/* Whether a matching IRP_MJ_CLEANUP was collected before CREATE retrieval. */
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

		/* Zero-filled trailing bytes used to align the next record to RecordAlignment. */
		uint64 PaddingBytes;
	};

	// Status and payload positions retain the version 1 wire layout.
	static_assert(sizeof(FFltIrpCreateOperationRecord) == 232);
	static_assert(offsetof(FFltIrpCreateOperationRecord, VolumeNameStatus) == 56);
	static_assert(offsetof(FFltIrpCreateOperationRecord, VolumeName) == 64);
	static_assert(offsetof(FFltIrpCreateOperationRecord, FilePathNameStatus) == 80);
	static_assert(offsetof(FFltIrpCreateOperationRecord, FilePathName) == 88);
	static_assert(offsetof(FFltIrpCreateOperationRecord, CompletionVolumeNameStatus) == 104);
	static_assert(offsetof(FFltIrpCreateOperationRecord, CompletionVolumeName) == 112);
	static_assert(offsetof(FFltIrpCreateOperationRecord, CompletionFilePathNameStatus) == 128);
	static_assert(offsetof(FFltIrpCreateOperationRecord, CompletionFilePathName) == 136);
}
