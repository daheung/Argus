#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"

class FDriverContext;

class FProcessCallback
{
public:
	// Inspect a process creation request and apply the creation policy.
	void OnProcessCreateNotify(PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo);

	// Check the policy for an exiting process and apply its cleanup actions.
	void OnProcessExitNotify(PEPROCESS Process, HANDLE ProcessId);

private:
	void ApplyRule(FDriverContext& Context, PEPROCESS Process, HANDLE ProcessId, PPS_CREATE_NOTIFY_INFO CreateInfo) noexcept;

private:
	class FProcessCreateRules
	{
	public:
		static void ApplyRule(
			FDriverContext& Context,
			PEPROCESS Process, 
			HANDLE ProcessId, 
			PPS_CREATE_NOTIFY_INFO CreateInfo
		) noexcept;

	private:
		static void ApplyIsolationProcessRule(
			FDriverContext& Context,
			PEPROCESS CreatorProcess,
			HANDLE ProcessId,
			PPS_CREATE_NOTIFY_INFO CreateInfo
		) noexcept;

		static void ApplyProtectedProcessRule(
			FDriverContext& Context,
			PEPROCESS CreatorProcess,
			HANDLE ProcessId,
			PPS_CREATE_NOTIFY_INFO CreateInfo
		) noexcept;
	};
};
