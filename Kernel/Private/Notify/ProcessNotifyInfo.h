#pragma once

#include "Core/Public/Notify/ProcessNotifyTypes.h"
#include "Core/Public/Templates/IsTriviallyRelocatable.h"
#include "Core/Public/Policy/PolicyTypes.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlString.h"

namespace Argus::Notify
{
	struct FProcessNotifyPolicy
	{
		Argus::Policy::EPolicyRole PolicyRole = Argus::Policy::EPolicyRole::None;
		Argus::Policy::EPolicyStatus PolicyStatus = Argus::Policy::EPolicyStatus::AccessPass;
	};

	struct FProcessNotifyInfo
	{
	public:
		EProcessNotifyType NotifyType = EProcessNotifyType::None;

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

		Argus::Policy::EPolicyRole PolicyRole = Argus::Policy::EPolicyRole::None;
		Argus::Policy::EPolicyStatus PolicyStatus = Argus::Policy::EPolicyStatus::AccessPass;

	public:
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
		void ResetStrings() noexcept
		{
			this->CommandLine.Empty();
			this->ImageFileName.Empty();
		}

		void MoveFrom(FProcessNotifyInfo& Other) noexcept
		{
			this->NotifyType		= Other.NotifyType;
			this->CreatedAt			= Other.CreatedAt;
			this->ExitedAt			= Other.ExitedAt;
			this->ProcessId			= Other.ProcessId;
			this->ParentProcessId	= Other.ParentProcessId;
			this->CreatingThreadId	= Other.CreatingThreadId;
			this->CreatingProcessId = Other.CreatingProcessId;
			this->ExitCode			= Other.ExitCode;
			this->CreationStatus	= Other.CreationStatus;
			this->PolicyRole		= Other.PolicyRole;
			this->PolicyStatus		= Other.PolicyStatus;

			FString<>::Swap(this->CommandLine  , Other.CommandLine);
			FString<>::Swap(this->ImageFileName, Other.ImageFileName);

			Other.NotifyType = EProcessNotifyType::None;
		}
	};
}

// Relocation transfers the FString allocations and their non-owning views.
template <>
struct TIsTriviallyRelocatable<Argus::Notify::FProcessNotifyInfo>
{
	static constexpr bool Value = true;
};
