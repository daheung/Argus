#pragma once

#include "Kernel/Private/Minifilter/Container/CreateOperationContainer.h"
#include "Kernel/Private/Minifilter/Container/CleanupOperationContainer.h"
#include "Kernel/Private/Minifilter/Container/CloseOperationContainer.h"

namespace Argus::Minifilter
{
	enum class EOperation : uint8
	{
		Create  = IRP_MJ_CREATE,
		Cleanup = IRP_MJ_CLEANUP,
		Close   = IRP_MJ_CLOSE,
	};
}

namespace Argus::Minifilter::Private
{
	template <EOperation Operation>
	struct TOperationContainerType;

	template <> struct TOperationContainerType<EOperation::Create > { using Type = FCreateOperationContainer; };
	template <> struct TOperationContainerType<EOperation::Cleanup> { using Type = FCleanupOperationContainer; };
	template <> struct TOperationContainerType<EOperation::Close  > { using Type = FCloseOperationContainer; };
}

class FMinifilterContainer
{
public:
	FORCEINLINE explicit FMinifilterContainer() noexcept
		: CreateOperationContainer()
		, bInitCreateOperationContainer(false)
		, CleanupOperationContainer()
		, bInitCleanupOperationContainer(false)
		, CloseOperationContainer()
		, bInitCloseOperationContainer(false)
	{}

public:
	NODISCARD bool Initialize() noexcept
	{
		bool bSuccess = true;

		if (CreateOperationContainer.Initialize())
		{
			bInitCreateOperationContainer = true;
		}
		bSuccess &= bInitCreateOperationContainer;

		if (CleanupOperationContainer.Initialize())
		{
			bInitCleanupOperationContainer = true;
		}
		bSuccess &= bInitCleanupOperationContainer;

		if (CloseOperationContainer.Initialize())
		{
			bInitCloseOperationContainer = true;
		}
		bSuccess &= bInitCloseOperationContainer;

		return bSuccess;
	}

	// auto = typename Argus::Minifilter::Private::TOperationContainerType<Operation>::Type
	template <Argus::Minifilter::EOperation Operation>
	NODISCARD auto& Get() noexcept;

private:
	FCreateOperationContainer CreateOperationContainer;
	bool bInitCreateOperationContainer;

	FCleanupOperationContainer CleanupOperationContainer;
	bool bInitCleanupOperationContainer;

	FCloseOperationContainer CloseOperationContainer;
	bool bInitCloseOperationContainer;
};

template <> FORCEINLINE auto& FMinifilterContainer::Get<Argus::Minifilter::EOperation::Create >() noexcept { return this->CreateOperationContainer; }
template <> FORCEINLINE auto& FMinifilterContainer::Get<Argus::Minifilter::EOperation::Cleanup>() noexcept { return this->CleanupOperationContainer; }
template <> FORCEINLINE auto& FMinifilterContainer::Get<Argus::Minifilter::EOperation::Close  >() noexcept { return this->CloseOperationContainer; }
