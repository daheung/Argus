#pragma once

#include "Kernel/Private/Minifilter/Container/CreateOperationContainer.h"

namespace Argus::Minifilter
{
	enum class EOperation : uint8
	{
		Create = IRP_MJ_CREATE,
	};
}

namespace Argus::Minifilter::Private
{
	template <EOperation Operation>
	struct TOperationContainerType;

	template <> struct TOperationContainerType<EOperation::Create> { using Type = FCreateOperationContainer; };
}

class FMinifilterContainer
{
public:
	NODISCARD bool Initialize() noexcept
	{
		return CreateOperationContainer.Initialize();
	}

	// auto = typename Argus::Minifilter::Private::TOperationContainerType<Operation>::Type
	template <Argus::Minifilter::EOperation Operation>
	NODISCARD auto& Get() noexcept;

private:
	FCreateOperationContainer CreateOperationContainer;
};

template <>
FORCEINLINE auto& FMinifilterContainer::Get<Argus::Minifilter::EOperation::Create>() noexcept
{
	return this->CreateOperationContainer;
}
