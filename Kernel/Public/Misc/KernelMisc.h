#pragma once

#include "Core/Public/DeviceControl/DeviceControlTypes.h"
#include "Core/Public/Policy/FilePolicyTypes.h"
#include "Core/Public/DeviceControl/Protocols/FltGetIrpOperationProtocol.h"
#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Public/Misc/KernelTypes.h"
#include "Kernel/Private/Minifilter/Container/CreateOperationContainer.h"

FORCEINLINE EProcessorMode ProcessorModeFrom(KPROCESSOR_MODE Mode) noexcept
{
	switch (Mode)
	{
	case UserMode:	 return EProcessorMode::UserMode;
	case KernelMode: return EProcessorMode::KernelMode;
	default:
		return EProcessorMode::Unknown;
	}
}

FORCEINLINE Argus::DeviceControl::EProcessorMode ProcessorModeTo(EProcessorMode Mode) noexcept
{
	switch (Mode)
	{
	case EProcessorMode::UserMode:   return Argus::DeviceControl::EProcessorMode::UserMode;
	case EProcessorMode::KernelMode: return Argus::DeviceControl::EProcessorMode::KernelMode;
	case EProcessorMode::Unknown:
	default:
		return Argus::DeviceControl::EProcessorMode::Unknown;
	}
}

namespace Argus::Minifilter::Private
{
	FORCEINLINE Argus::DeviceControl::EFltIrpPreOperationDecision PreDecisionEnumTo(Argus::Minifilter::ECreatePreOperationDecision Decision) noexcept
	{
		using namespace Argus::Minifilter;
		using namespace Argus::DeviceControl;

		switch (Decision)
		{
		case ECreatePreOperationDecision::PreOperationPass:	   return EFltIrpPreOperationDecision::PreOperationPass;
		case ECreatePreOperationDecision::PreOperationBlock:   return EFltIrpPreOperationDecision::PreOperationBlock;
		case ECreatePreOperationDecision::PreOperationPending: return EFltIrpPreOperationDecision::PreOperationPendingDecision;
		default:
			return EFltIrpPreOperationDecision::PreOperationNone;
		}
	}

	FORCEINLINE Argus::DeviceControl::EFltIrpPostOperationDecision PostDecisionEnumTo(Argus::Minifilter::ECreatePostOperationDecision Decision) noexcept
	{
		using namespace Argus::Minifilter;
		using namespace Argus::DeviceControl;

		switch (Decision)
		{
		case ECreatePostOperationDecision::PostOperationPass:  return EFltIrpPostOperationDecision::PostOperationPass;
		case ECreatePostOperationDecision::PostOperationBlock: return EFltIrpPostOperationDecision::PostOperationBlock;
		case ECreatePostOperationDecision::PostOperationMoreProcessingRequired: return EFltIrpPostOperationDecision::PostOperationPendingDecision;
		default:
			return EFltIrpPostOperationDecision::PostOperationNone;
		}
	}
};

FORCEINLINE Argus::DeviceControl::FFltIrpPreOperationDecision PreDecisionTo(Argus::Minifilter::FCreatePreOperationDecision Decision) noexcept
{
	Argus::DeviceControl::FFltIrpPreOperationDecision OutDecision{};
	OutDecision.Decision = Argus::Minifilter::Private::PreDecisionEnumTo(Decision.Decision);
	OutDecision.BlockStatus = static_cast<uint32>(Decision.BlockStatus);
	return OutDecision;
}

FORCEINLINE Argus::DeviceControl::FFltIrpPostOperationDecision PostDecisionTo(Argus::Minifilter::FCreatePostOperationDecision Decision) noexcept
{
	Argus::DeviceControl::FFltIrpPostOperationDecision OutDecision{};
	OutDecision.Decision = Argus::Minifilter::Private::PostDecisionEnumTo(Decision.Decision);
	OutDecision.BlockStatus = static_cast<uint32>(Decision.BlockStatus);
	return OutDecision;
}