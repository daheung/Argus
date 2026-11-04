#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Core/Public/Templates/Atomic.h"

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
	FMinifilter() noexcept
	{
		KeInitializeEvent(&UnregisterComplete, NotificationEvent, FALSE);
	}
	FMinifilter(const FMinifilter&) = delete;
	FMinifilter& operator=(const FMinifilter&) = delete;

	// Start and destruction must be serialized with unregister requests.
	// Concurrent UnregisterFilter callers wait for the same teardown to finish.
	NTSTATUS Start(PDRIVER_OBJECT DriverObject) noexcept;
	
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

private:
	PFLT_FILTER Filter = nullptr;
	bool bStartAttempted = false;
	KEVENT UnregisterComplete;

	TKernelAtomicCounter<uint32> FilterMode{ static_cast<uint32>(EFilterMode::None) };
	TKernelAtomicCounter<uint32> FilterState{ static_cast<uint32>(EFilterState::Unregistered) };
};
