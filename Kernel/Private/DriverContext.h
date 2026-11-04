#pragma once

#include "Kernel/Private/Notify/ProcessNotifyQueue.h"
#include "Kernel/Private/Notify/ThreadNotifyQueue.h"
#include "Kernel/Private/Notify/ImageNotifyQueue.h"
#include "Kernel/Private/Notify/RegistryNotifyQueue.h"
#include "Kernel/Private/Notify/ObjectOperationNotifyQueue.h"
#include "Kernel/Private/Minifilter/Minifilter.h"
#include "Kernel/Private/Protection/AgentState.h"
#include "Kernel/Private/Callback/ProcessCallback.h"
#include "Kernel/Private/Callback/ObjectCallback.h"
#include "Kernel/Private/Callback/RegistryCallback.h"
#include "Kernel/Private/Protection/ProtectedProcessRegistry.h"
#include "Kernel/Private/Protection/IsolationProcessRegistry.h"

class FDriverContext
{
public:
	explicit FDriverContext(uint32 Tag) noexcept
		: ProcessNotifyQueue(Tag)
		, ThreadNotifyQueue(Tag)
		, ImageNotifyQueue(Tag)
		, RegistryNotifyQueue(Tag)
		, ObjectOperationNotifyQueue(Tag)
	{
	}

	FDriverContext(const FDriverContext&) = delete;
	FDriverContext& operator=(const FDriverContext&) = delete;

	NODISCARD FORCEINLINE FProcessNotifyQueue& GetProcessNotifyQueue() noexcept
	{
		return this->ProcessNotifyQueue;
	}
	NODISCARD FORCEINLINE FThreadNotifyQueue& GetThreadNotifyQueue() noexcept
	{
		return this->ThreadNotifyQueue;
	}
	NODISCARD FORCEINLINE FImageNotifyQueue& GetImageNotifyQueue() noexcept
	{
		return this->ImageNotifyQueue;
	}
	NODISCARD FORCEINLINE FRegistryNotifyQueue& GetRegistryNotifyQueue() noexcept
	{
		return this->RegistryNotifyQueue;
	}
	NODISCARD FORCEINLINE FObjectOperationNotifyQueue& GetObjectOperationNotifyQueue() noexcept
	{
		return this->ObjectOperationNotifyQueue;
	}

	NODISCARD FORCEINLINE FMinifilter& GetMinifilter() noexcept
	{
		return this->Minifilter;
	}

	NODISCARD FORCEINLINE FProcessCallback& GetProcessCallback() noexcept
	{
		return this->ProcessCallback;
	}
	NODISCARD FORCEINLINE FObjectCallback& GetObjectCallback() noexcept
	{
		return this->ObjectCallback;
	}
	NODISCARD FORCEINLINE FRegistryCallback& GetRegistryCallback() noexcept
	{
		return this->RegistryCallback;
	}

	FORCEINLINE FAgentState& GetAgentState() noexcept
	{
		return this->AgentState;
	}

	FORCEINLINE FProtectedProcessRegistry& GetProtectedProcessRegistry() noexcept
	{
		return this->ProtectedProcessRegistry;
	}

	FORCEINLINE FIsolationProcessRegistry& GetIsolationProcessRegistry() noexcept
	{
		return this->IsolationProcessRegistry;
	}

public:
	NTSTATUS InitializeCallbacks(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath) noexcept;

	void ShutdownCallbacks() noexcept;

public:
	void SetDeviceObject(PDEVICE_OBJECT InDeviceObject) noexcept
	{
		this->DeviceObject = InDeviceObject;
	}

	PDEVICE_OBJECT GetDeviceObject() const noexcept
	{
		return this->DeviceObject;
	}

	void SetSymbolicLinkCreated(bool bCreated) noexcept
	{
		this->bSymbolicLinkCreated = bCreated;
	}

	bool IsSymbolicLinkCreated() const noexcept
	{
		return this->bSymbolicLinkCreated;
	}

private:
	FProcessNotifyQueue ProcessNotifyQueue;
	FThreadNotifyQueue ThreadNotifyQueue;
	FImageNotifyQueue ImageNotifyQueue;
	FRegistryNotifyQueue RegistryNotifyQueue;
	FObjectOperationNotifyQueue ObjectOperationNotifyQueue;

	FMinifilter Minifilter;

	FProcessCallback ProcessCallback;
	FObjectCallback ObjectCallback;
	FRegistryCallback RegistryCallback;

	FAgentState AgentState;
	FProtectedProcessRegistry ProtectedProcessRegistry;
	FIsolationProcessRegistry IsolationProcessRegistry;
	
	PDEVICE_OBJECT DeviceObject = nullptr;
	
	bool bSymbolicLinkCreated = false;
	bool bProcessNotifyRegistered = false;
	bool bThreadNotifyRegistered = false;
	bool bImageNotifyRegistered = false;
	bool bRegistryCallbackRegistered = false;

	LARGE_INTEGER CmCallbackRegistryCookie{};
	PVOID ObCallbackRegistrationHandle = nullptr;
};

extern FDriverContext* GDriverContext;
