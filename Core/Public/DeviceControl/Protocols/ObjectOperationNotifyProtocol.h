#pragma once

#include "Core/Public/Notify/ObjectOpNotifyTypes.h"

namespace Argus::DeviceControl
{
	struct FObjectOperationRP
	{
		uint32 Version;
		uint32 CurRecordCount;
		uint32 NextRecordCount;
	};

	struct FObjectOperationNotifyHeader
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
		uint32 Reserved;
	};

	struct FObjectOperationNotifyRecord
	{
	public:
		int64 PreOperationAt;
		int64 PostOperationAt;

		Argus::Notify::EObjectOperationType OperationType;
		Argus::Notify::EObjectTargetType TargetType;

		uint32 RequesterProcessId;
		uint32 RequesterThreadId;
		uint32 HandleOwnerProcessId;

		// EObjectOperationType::Duplicate
		uint32 SourceProcessId;
		uint32 TargetProcessId;

		// EObjectTargetType::Thread
		uint32 TargetThreadId;

		uint32 OriginalDesiredAccess;
		uint32 DesiredAccessBeforePolicy;
		uint32 DesiredAccessAfterPolicy;
		uint32 GrantedAccess;

		uint32 ReturnStatus;
	};
}