#pragma once

#include "Core/Public/Notify/RegistryNotifyTypes.h"
#include "Core/Public/Templates/IsTriviallyRelocatable.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlString.h"

namespace Argus::Notify
{
	/**
	 * @brief Owns one registry notification and its optional UTF-16 names.
	 *
	 * KeyName and ValueName own their storage independently. The name statuses
	 * distinguish an unavailable name from a captured empty value name. The
	 * producer must copy names before its callback returns and must never retain
	 * pointers into registry callback structures.
	 */
	class FRegistryNotifyInfo
	{
	public:
		FRegistryNotifyInfo() noexcept = default;
		~FRegistryNotifyInfo() noexcept = default;

		FRegistryNotifyInfo(const FRegistryNotifyInfo&) = delete;
		FRegistryNotifyInfo& operator=(const FRegistryNotifyInfo&) = delete;

		FRegistryNotifyInfo(FRegistryNotifyInfo&& Other) noexcept
		{
			this->MoveFrom(Other);
		}

		FRegistryNotifyInfo& operator=(FRegistryNotifyInfo&& Other) noexcept
		{
			if (this != &Other)
			{
				this->ResetNames();
				this->MoveFrom(Other);
			}
			return *this;
		}

		/** @brief Releases both names and clears their capture statuses. */
		void ResetNames() noexcept
		{
			FString<> EmptyKeyName;
			FString<> EmptyValueName;
			FNameArray::Swap(this->KeyName.GetCharArray(), EmptyKeyName.GetCharArray());
			FNameArray::Swap(this->ValueName.GetCharArray(), EmptyValueName.GetCharArray());
			this->KeyNameStatus = ERegistryNameStatus::Unavailable;
			this->ValueNameStatus = ERegistryNameStatus::NotApplicable;
		}

	public:
		ERegistryOperationType OperationType = ERegistryOperationType::Unknown;
		ERegistryNotifyPhase Phase = ERegistryNotifyPhase::Unknown;
		ERegistryDecision Decision = ERegistryDecision::Unknown;
		ERegistryRequesterRole RequesterRoles = ERegistryRequesterRole::None;
		ERegistryNameStatus KeyNameStatus = ERegistryNameStatus::Unavailable;
		ERegistryNameStatus ValueNameStatus = ERegistryNameStatus::NotApplicable;

		LARGE_INTEGER OccurredAt{};
		uint64 OperationId = 0;
		uint32 ProcessId = 0;
		uint32 ThreadId = 0;
		uint32 IsolationFlags = 0;

		// Pre: callback decision status. Post: completed registry operation status.
		NTSTATUS Status = STATUS_SUCCESS;

		FString<> KeyName;
		FString<> ValueName;

	private:
		using FNameArray = TArray<EPoolType::Paged, WIDECHAR>;

		void MoveFrom(FRegistryNotifyInfo& Other) noexcept
		{
			this->OperationType = Other.OperationType;
			this->Phase = Other.Phase;
			this->Decision = Other.Decision;
			this->RequesterRoles = Other.RequesterRoles;
			this->KeyNameStatus = Other.KeyNameStatus;
			this->ValueNameStatus = Other.ValueNameStatus;
			this->OccurredAt = Other.OccurredAt;
			this->OperationId = Other.OperationId;
			this->ProcessId = Other.ProcessId;
			this->ThreadId = Other.ThreadId;
			this->IsolationFlags = Other.IsolationFlags;
			this->Status = Other.Status;
			FNameArray::Swap(this->KeyName.GetCharArray(), Other.KeyName.GetCharArray());
			FNameArray::Swap(this->ValueName.GetCharArray(), Other.ValueName.GetCharArray());

			Other.KeyNameStatus = ERegistryNameStatus::Unavailable;
			Other.ValueNameStatus = ERegistryNameStatus::NotApplicable;
		}
	};
}

// Relocation transfers both owned string allocations without duplicating them.
template <>
struct TIsTriviallyRelocatable<Argus::Notify::FRegistryNotifyInfo>
{
	static constexpr bool Value = true;
};
