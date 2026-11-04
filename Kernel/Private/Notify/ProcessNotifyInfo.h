#pragma once

#include "Core/Public/Notify/ProcessNotifyTypes.h"
#include "Core/Public/Templates/IsTriviallyRelocatable.h"
#include "Kernel/Public/Misc/KernelDefines.h"

namespace Argus::Notify
{
	struct FProcessNotifyInfo
	{
		EProcessNotifyType NotifyType = EProcessNotifyType::Unknown;
		LARGE_INTEGER CreatedAt{};
		LARGE_INTEGER ExitedAt{};
		uint32 ProcessId = 0;
		uint32 ParentProcessId = 0;
		uint32 CreatingThreadId = 0;
		uint32 CreatingProcessId = 0;
		uint16 CommandLineChars = 0;
		PWCHAR CommandLine = nullptr;

		//EImageFileNameStatus ImageFileNameStatus = EImageFileNameStatus::None;
		uint16 ImageFileNameChars = 0;
		PWCHAR ImageFileName = nullptr;
		uint32 ExitCode = 0;
		uint32 CreationStatus = STATUS_SUCCESS;

		FORCEINLINE bool operator==(const FProcessNotifyInfo& Other) const noexcept
		{
			return ProcessId == Other.ProcessId;
		}

		FProcessNotifyInfo() = default;
		FProcessNotifyInfo(const FProcessNotifyInfo&) = delete;
		FProcessNotifyInfo& operator=(const FProcessNotifyInfo&) = delete;

		FProcessNotifyInfo(FProcessNotifyInfo&& Other) noexcept
			: NotifyType(Other.NotifyType)
			, CreatedAt(Other.CreatedAt)
			, ExitedAt(Other.ExitedAt)
			, ProcessId(Other.ProcessId)
			, ParentProcessId(Other.ParentProcessId)
			, CreatingThreadId(Other.CreatingThreadId)
			, CreatingProcessId(Other.CreatingProcessId)
			, CommandLineChars(Other.CommandLineChars)
			, CommandLine(Other.CommandLine)
			, ImageFileNameChars(Other.ImageFileNameChars)
			, ImageFileName(MoveTemp(Other.ImageFileName))
			, ExitCode(Other.ExitCode)
			, CreationStatus(Other.CreationStatus)
		{
			Other.NotifyType		 = EProcessNotifyType::Unknown;
			Other.CommandLineChars	 = 0;
			Other.CommandLine		 = nullptr;
			Other.ImageFileNameChars = 0;
			Other.ImageFileName		 = nullptr;
		}

		FProcessNotifyInfo& operator=(FProcessNotifyInfo&& Other) noexcept
		{
			if (this != &Other)
			{
				if (CommandLine != nullptr)
				{
					ExFreePool(CommandLine);
				}
				if (ImageFileName != nullptr)
				{
					ExFreePool(ImageFileName);
				}

				NotifyType		   = Other.NotifyType;
				CreatedAt		   = Other.CreatedAt;
				ExitedAt		   = Other.ExitedAt;
				ProcessId		   = Other.ProcessId;
				ParentProcessId	   = Other.ParentProcessId;
				CreatingThreadId   = Other.CreatingThreadId;
				CreatingProcessId  = Other.CreatingProcessId;
				CommandLineChars   = Other.CommandLineChars;
				CommandLine		   = Other.CommandLine;
				ImageFileNameChars = Other.ImageFileNameChars;
				ImageFileName	   = Other.ImageFileName;
				ExitCode		   = Other.ExitCode;
				CreationStatus	   = Other.CreationStatus;

				Other.NotifyType		 = EProcessNotifyType::Unknown;
				Other.CommandLineChars	 = 0;
				Other.CommandLine		 = nullptr;
				Other.ImageFileNameChars = 0;
				Other.ImageFileName		 = nullptr;
			}
			return *this;
		}

		~FProcessNotifyInfo() noexcept
		{
			if (CommandLine != nullptr)
			{
				ExFreePool(CommandLine);
			}
			if (ImageFileName != nullptr)
			{
				ExFreePool(ImageFileName);
			}
		}
	};
}

// The strings live in separate pool allocations. Relocation transfers their
// pointers and abandons the old record without invoking its destructor.
template <>
struct TIsTriviallyRelocatable<Argus::Notify::FProcessNotifyInfo>
{
	static constexpr bool Value = true;
};
