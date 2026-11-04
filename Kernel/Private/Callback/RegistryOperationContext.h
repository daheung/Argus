#pragma once

#include "Core/Public/Notify/RegistryNotifyTypes.h"
#include "Kernel/Public/Misc/KernelDefines.h"

/**
 * @brief Transient, decoded view of one registry callback invocation.
 *
 * Information is owned by the configuration manager and is valid only for
 * this callback invocation. The notify queue must copy any names it retains.
 * Roles describe the thread's owning process, not the registry key.
 */
struct FRegistryOperationContext
{
	REG_NOTIFY_CLASS NotifyClass = MaxRegNtNotifyClass;
	Argus::Notify::ERegistryOperationType OperationType = Argus::Notify::ERegistryOperationType::Unknown;
	Argus::Notify::ERegistryNotifyPhase Phase = Argus::Notify::ERegistryNotifyPhase::Unknown;
	Argus::Notify::ERegistryRequesterRole RequesterRoles = Argus::Notify::ERegistryRequesterRole::None;
	uint32 ProcessId = 0;
	uint32 ThreadId = 0;
	uint32 IsolationFlags = 0;
	void* Information = nullptr;

	bool HasRole(Argus::Notify::ERegistryRequesterRole Role) const noexcept
	{
		return (static_cast<uint8>(this->RequesterRoles) & static_cast<uint8>(Role)) != 0;
	}
};

class FDriverContext;

/** @brief Decode one invocation for callback policy or notify collection. */
FRegistryOperationContext BuildRegistryOperationContext(FDriverContext& Context, void* ArgumentA, void* ArgumentB) noexcept;
