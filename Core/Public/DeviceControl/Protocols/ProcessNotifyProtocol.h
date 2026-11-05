#pragma once

#include "Core/Public/Notify/ProcessNotifyTypes.h"
#include "Core/Public/Policy/PolicyTypes.h"

namespace Argus::DeviceControl
{
	enum class EImageFileNameStatus : uint32
	{
		None,
		WriteSucceed,
		WriteFailed,
		NotFound
	};

	struct FProcessNotifyRP
	{
		uint32 Version;
		uint32 CurRecordCount;
		uint32 NextRecordCount;
	};

	struct FProcessNotifyHeader
	{
	public:
		static constexpr uint32 HeaderVersion = 2u;

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

	struct FProcessNotifyRecord
	{
	public:
		static constexpr uint32 RecordAlignment = 4u;

	public:
		uint64 RecordHeaderBytes;

		Argus::Notify::EProcessNotifyType NotifyType;
		Argus::Policy::EPolicyRole PolicyRole;
		Argus::Policy::EPolicyStatus PolicyStatus;

		// UTC time in 100-nanosecond intervals since January 1, 1601. Zero means unavailable.
		int64 CreatedAt;
		int64 ExitedAt;
		uint32 ProcessId;
		uint32 ParentProcessId;
		uint32 CreatingThreadId;
		uint32 CreatingProcessId;
		uint32 CreationStatus;
		uint32 ExitCode;
		
		uint64 CommandLineBytes;
		uint64 CommandLineOffset;

		uint64 ImageFileNameBytes;
		uint64 ImageFileNameOffset;

		uint64 PaddingBytes;
	};
}
