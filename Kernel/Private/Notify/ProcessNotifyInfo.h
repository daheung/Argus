#pragma once

#include "Core/Public/Notify/ProcessNotifyTypes.h"
#include "Core/Public/Templates/IsTriviallyRelocatable.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlString.h"

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
		FString<> CommandLine;
		FString<> ImageFileName;

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
		{
			this->MoveFrom(Other);
		}

		FProcessNotifyInfo& operator=(FProcessNotifyInfo&& Other) noexcept
		{
			if (this != &Other)
			{
				this->ResetStrings();
				this->MoveFrom(Other);
			}
			return *this;
		}

		~FProcessNotifyInfo() noexcept = default;

	private:
		using FStringArray = TArray<EPoolType::Paged, WIDECHAR>;

		void ResetStrings() noexcept
		{
			FString<> EmptyCommandLine;
			FString<> EmptyImageFileName;
			FStringArray::Swap(this->CommandLine.GetCharArray(), EmptyCommandLine.GetCharArray());
			FStringArray::Swap(this->ImageFileName.GetCharArray(), EmptyImageFileName.GetCharArray());
		}

		void MoveFrom(FProcessNotifyInfo& Other) noexcept
		{
			this->NotifyType = Other.NotifyType;
			this->CreatedAt = Other.CreatedAt;
			this->ExitedAt = Other.ExitedAt;
			this->ProcessId = Other.ProcessId;
			this->ParentProcessId = Other.ParentProcessId;
			this->CreatingThreadId = Other.CreatingThreadId;
			this->CreatingProcessId = Other.CreatingProcessId;
			this->ExitCode = Other.ExitCode;
			this->CreationStatus = Other.CreationStatus;
			FStringArray::Swap(this->CommandLine.GetCharArray(), Other.CommandLine.GetCharArray());
			FStringArray::Swap(this->ImageFileName.GetCharArray(), Other.ImageFileName.GetCharArray());
			Other.NotifyType = EProcessNotifyType::Unknown;
		}
	};
}

// Relocation transfers the FString allocations and their non-owning views.
template <>
struct TIsTriviallyRelocatable<Argus::Notify::FProcessNotifyInfo>
{
	static constexpr bool Value = true;
};
