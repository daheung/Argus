#pragma once
#include "Core/Public/Notify/RegistryNotifyTypes.h"

namespace Argus::DeviceControl
{
	struct FRegistryNotifyHeader
	{
	public:
		static constexpr uint32 HeaderVerion = 1u;

	public:
		uint32 Version;
		uint64 HeaderBytes;
		uint64 TotalBytes;
		uint64 DropCount;
		uint32 NextRecordCount;
		uint64 NextRecordBytes;
		uint32 Reserved;
	};

	struct FRegistryNotifyRecord
	{
	public:
		static constexpr uint32 RecordAlignment = 8u;

	public:
		uint64 RecordHeaderBytes;
		uint64 RecordBytes;
		uint64 OperationId;

		Argus::Notify::ERegistryRequesterRole RequesterRoles;
		Argus::Notify::ERegistryOperationType OperationType;
		Argus::Notify::ERegistryDecision Decision;
		Argus::Notify::ERegistryNotifyPhase Phase;
		Argus::Notify::ERegistryNameStatus KeyNameStatus;
		Argus::Notify::ERegistryNameStatus ValueNameStatus;
		
		int64 OccurredAt;
		int32 Status;
		uint32 ProcessId;
		uint32 ThreadId;
		uint32 IsolationFlags;

		uint64 KeyNameOffset;
		uint64 KeyNameBytes;
		uint64 ValueNameOffset;
		uint64 ValueNameBytes;

		uint64 PaddingBytes;
	};

	static_assert(sizeof(FRegistryNotifyHeader) == 56,
		"Registry notify header layout must match in kernel and user mode");
	static_assert(sizeof(FRegistryNotifyRecord) == 96,
		"Registry notify record layout must match in kernel and user mode");
}
