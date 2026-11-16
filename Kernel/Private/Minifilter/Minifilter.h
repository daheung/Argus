#pragma once

#include "Core/Public/Policy/FilePolicyTypes.h"
#include "Core/Public/Templates/Atomic.h"
#include "Kernel/Private/Minifilter/MinifilterTypes.h"
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

public:
	/**
	 * @brief Attaches a stream-handle context to a successful create operation.
	 * @param Data Callback data used to identify the requesting process.
	 * @param Objects Filter objects containing the instance and file object.
	 * @param Descriptor Create record supplying the requester PID and observation time.
	 * @return The attached context ID, or the existing context ID if one is already set;
	 *         0 if stream-handle contexts are unsupported or the operation fails.
	 */
	uint64 AttachStreamHandleContext(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS Objects, const Argus::Minifilter::FCreateOperationDescriptor& Descriptor) noexcept;

	/**
	 * @brief Copies the stream-handle context attached to the given file object.
	 * @param Objects Filter objects containing the instance and file object.
	 * @param OutContext Receives a context snapshot on success; zeroed on failure.
	 * @return STATUS_SUCCESS on success, STATUS_INVALID_PARAMETER for invalid objects,
	 *         or the status returned by FltGetStreamHandleContext.
	 * @pre IRQL must be at most APC_LEVEL.
	 */
	NTSTATUS ReadStreamHandleContext(PCFLT_RELATED_OBJECTS Objects, Argus::Minifilter::FMinifilterContext& OutContext) noexcept;

	/**
	 * @brief Publishes a complete stream-handle context, replacing an existing one.
	 * @param Objects Filter objects containing the filter, instance, and file object.
	 * @param InContext Context value to publish; inserted if none is attached yet.
	 * @return STATUS_SUCCESS on success, or the allocation or Filter Manager error.
	 * @pre IRQL must be at most APC_LEVEL.
	 */
	NTSTATUS WriteStreamHandleContext(PCFLT_RELATED_OBJECTS Objects, const Argus::Minifilter::FMinifilterContext& InContext) noexcept;

private:
	PFLT_FILTER Filter;
	bool bStartAttempted;
	KEVENT UnregisterComplete;

	FMinifilterVolumeContext VolumeContext;

	FMinifilterPolicyRegistry FilterPolicyRegistry;

	TKernelAtomicCounter<uint32> FilterMode{ static_cast<uint32>(EFilterMode::None) };
	TKernelAtomicCounter<uint32> FilterState{ static_cast<uint32>(EFilterState::Unregistered) };
	
	TKernelAtomicCounter<uint64> NextOperationId{ 1 };

	FMinifilterContainer FilterContainer;
};
