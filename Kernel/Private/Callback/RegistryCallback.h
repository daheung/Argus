#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Kernel/Private/Callback/RegistryOperationContext.h"

class FDriverContext;

class FRegistryCallback
{
public:
	// Decode and apply policy. Post callbacks return STATUS_SUCCESS.
	NTSTATUS HandleRegistryCallback(FDriverContext& Context, void* ArgumentA, void* ArgumentB) noexcept;

private:
	NTSTATUS OnPreRegistryCallback(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept;
	void OnPostRegistryCallback(const FRegistryOperationContext& Operation) noexcept;

	NTSTATUS ApplySetKeyInformationRule(const FRegistryOperationContext& Operation) noexcept;
	NTSTATUS ApplySetValueRule(const FRegistryOperationContext& Operation) noexcept;
	NTSTATUS ApplyDeleteKeyRule(const FRegistryOperationContext& Operation) noexcept;
	NTSTATUS ApplyDeleteValueRule(const FRegistryOperationContext& Operation) noexcept;
};

class FRegistryCreateKeyRules
{
public:
	static NTSTATUS ApplyRule(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept;

private:
	static NTSTATUS ApplyAgentProcessRule(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept;

	static NTSTATUS ApplyProtectedProcessRule(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept;

	static NTSTATUS ApplyIsolationProcessRule(FDriverContext& Context, const FRegistryOperationContext& Operation) noexcept;
};