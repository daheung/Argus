#pragma once

#include "Core/Public/Notify/ProcessNotifyTypes.h"

namespace Argus::DeviceControl
{
	enum class EImageFileNameStatus : uint32
	{
		None,
		WriteSucceed,
		WriteFailed,
		NotFound
	};

	//struct FWCharBuffer {};

	// One output slot contains the record, command line, and executable base name.
	struct FProcessNotifyRecord
	{
		Argus::Notify::EProcessNotifyType NotifyType;
		// UTC time in 100-nanosecond intervals since January 1, 1601. Zero means unavailable.
		int64 CreatedAt;
		int64 ExitedAt;
		uint32 ProcessId;
		uint32 ParentProcessId;
		uint32 CreatingThreadId;
		uint32 CreatingProcessId;
		uint32 CreationStatus;
		uint32 ExitCode;
		uint32 CommandLineBytes;
		uint32 ImageFileNameBytes;
	};

	struct FProcessNotifySlot
	{
		FProcessNotifyRecord Record;
		uint8 CommandLine[Argus::Notify::ProcessNotifyMaxCommandLineBytes];
		WIDECHAR ImageFileName[Argus::Notify::ProcessNotifyMaxImageFileNameBytes / sizeof(WIDECHAR)];
		//FWCharBuffer CommandLine;
		//FWCharBuffer ImageFileName;
	};

	static_assert(sizeof(FProcessNotifyRecord) == 56,
		"Process notify record layout must match in kernel and user mode");
	static_assert(sizeof(FProcessNotifySlot) ==
		sizeof(FProcessNotifyRecord) + Argus::Notify::ProcessNotifyMaxCommandLineBytes +
		Argus::Notify::ProcessNotifyMaxImageFileNameBytes,
		"Process notify slots must have a fixed wire size");
}
