#pragma once

#include "Core/Public/Policy/FilePolicyTypes.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Containers/KrnlArray.h"
#include "Kernel/Public/Containers/KrnlContainerForward.h"
#include "Kernel/Private/Minifilter/MinifilterPolicyRegistry.h"
#include "Kernel/Private/Minifilter/MinifilterContainer.h"
#include "Kernel/Private/Minifilter/Volume/Volume.h"

enum class EFilterMode : uint32
{
	None,
	Pass,
	Passive,
	Active,
};

enum class EFilterState : uint32
{
	Unregistered  = 0x01,
	Registered	  = 0x02,
	Unregistering = 0x04,
};

class FMinifilter
{
public:
	typedef TArray<EPoolType::NonPaged, Argus::Policy::FMinifilterPolicyRuleDescriptor> FNonPFileObjectRules;

public:
	FMinifilter() noexcept
		: Filter(nullptr)
		, bStartAttempted(false)
		, UnregisterComplete()
		, FilterPolicyRegistry()
	{
		KeInitializeEvent(&UnregisterComplete, NotificationEvent, FALSE);
	}

	FMinifilter(const FMinifilter&) = delete;
	FMinifilter& operator=(const FMinifilter&) = delete;

	// Start and destruction must be serialized with unregister requests.
	// Concurrent UnregisterFilter callers wait for the same teardown to finish.
	NTSTATUS InitStart(PDRIVER_OBJECT DriverObject) noexcept;
	
	void UnregisterFilter() noexcept;

	NTSTATUS SetFilterMode(EFilterMode NewMode) noexcept;

public:
	NODISCARD FORCEINLINE EFilterMode GetFilterMode() const noexcept
	{
		return static_cast<EFilterMode>(FilterMode.load(Atomic::EMemoryOrder::Acquire));
	}

	NODISCARD FORCEINLINE EFilterState GetFilterState() const noexcept
	{
		return static_cast<EFilterState>(FilterState.load(Atomic::EMemoryOrder::Acquire));
	}

	NODISCARD FORCEINLINE FMinifilterContainer& GetOperationContainer() noexcept
	{
		return FilterContainer;
	}

	NODISCARD FORCEINLINE FMinifilterVolumeContext& GetVolumeContext() noexcept
	{
		return VolumeContext;
	}

private:
	PFLT_FILTER Filter;
	bool bStartAttempted;
	KEVENT UnregisterComplete;

	FMinifilterVolumeContext VolumeContext;

	FMinifilterPolicyRegistry FilterPolicyRegistry;

	TKernelAtomicCounter<uint32> FilterMode{ static_cast<uint32>(EFilterMode::None) };
	TKernelAtomicCounter<uint32> FilterState{ static_cast<uint32>(EFilterState::Unregistered) };

	FMinifilterContainer FilterContainer;
};
