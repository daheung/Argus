#pragma once

#include "Core/Public/Policy/FilePolicyTypes.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Private/Notify/LockType.h"

class FMinifilterPolicyRegistry
{
public:
	typedef TArray<EPoolType::NonPaged, Argus::Policy::FMinifilterPolicyRuleDescriptor> TRuleArray;

public:
	FMinifilterPolicyRegistry() noexcept
		: PolicyRules('tlFk')
	{
		ExInitializePushLock(&this->PolicyRulesLock);
	}

public:
	NODISCARD FORCEINLINE TRuleArray& GetQueue() noexcept
	{
		return this->PolicyRules;
	}

	FORCEINLINE void Lock(ELockType LockType) noexcept
	{
		KeEnterCriticalRegion();

		if (LockType == ELockType::Shared)
		{
			ExAcquirePushLockShared(&this->PolicyRulesLock);
		}
		else
		{
			ExAcquirePushLockExclusive(&this->PolicyRulesLock);
		}
	}

	FORCEINLINE void Unlock(ELockType LockType) noexcept
	{
		if (LockType == ELockType::Shared)
		{
			ExReleasePushLockShared(&this->PolicyRulesLock);
		}
		else
		{
			ExReleasePushLockExclusive(&this->PolicyRulesLock);
		}

		KeLeaveCriticalRegion();
	}

private:
	TRuleArray PolicyRules;
	EX_PUSH_LOCK PolicyRulesLock;
};