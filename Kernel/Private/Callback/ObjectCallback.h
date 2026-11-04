#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"

class FDriverContext;

class FObjectCallback
{
public:
	static PVOID GetTerminateAccessRemovedContext() noexcept;

	/**
	 * @brief Applies policies before a user handle to a process or thread is created or duplicated.
	 * @param RegistrationContext The FDriverContext supplied at callback registration.
	 * @param Information The object and requested access for the handle operation.
	 * @return OB_PREOP_SUCCESS after the applicable policies have been evaluated.
	 */
	OB_PREOP_CALLBACK_STATUS OnPreOperationCallback(void* RegistrationContext, POB_PRE_OPERATION_INFORMATION Information) noexcept;

	/**
	 * @brief Checks whether termination access was granted despite pre-callback removal.
	 *
	 * Thread handles are not audited by this function.
	 * @param RegistrationContext The FDriverContext supplied at callback registration.
	 * @param Information The completed handle operation and its granted access.
	 */
	void OnPostOperationCallback(void* RegistrationContext, POB_POST_OPERATION_INFORMATION Information) noexcept;

private:
	/**
	 * @brief Gets the process that will receive the new handle.
	 *
	 * For duplication, this is the destination process, which can differ from
	 * the process that initiated the request.
	 * @param Information The pending handle operation.
	 * @return The receiving process, or nullptr for an unsupported operation.
	 */
	static PEPROCESS GetHandleOwnerProcess(POB_PRE_OPERATION_INFORMATION Information) noexcept;

	/**
	 * @brief Routes a pending handle operation to the process or thread rules.
	 * @param Context The driver state used by the rules.
	 * @param Information The pending operation and its target object.
	 */
	void ApplyPreOperationRules(FDriverContext& Context, POB_PRE_OPERATION_INFORMATION Information) noexcept;

private:
	class FPreOperationProcessRules
	{
	public:
		/**
		 * @brief Applies matching policies to a handle targeting a process.
		 * @param Context The driver state containing Agent and process registries.
		 * @param HandleOwnerProcess The process receiving the handle.
		 * @param TargetProcess The process referenced by the handle.
		 * @param Information The pending operation and modifiable requested access.
		 */
		static void ApplyRule(FDriverContext& Context, PEPROCESS HandleOwnerProcess, PEPROCESS TargetProcess, OB_PRE_OPERATION_INFORMATION& Information) noexcept;

	private:
		/**
		 * @brief Removes termination access from another process's handle to the Agent.
		 * @param Context The driver state; unused by this rule.
		 * @param HandleOwnerProcess The process receiving the handle.
		 * @param TargetProcess The Agent process referenced by the handle.
		 * @param Information The pending operation and modifiable requested access.
		 */
		static void ApplyAgentProcessRule(FDriverContext& Context, PEPROCESS HandleOwnerProcess, PEPROCESS TargetProcess, OB_PRE_OPERATION_INFORMATION& Information) noexcept;
		
		/**
		 * @brief Removes selected rights from another process's handle to a protected process.
		 * @param Context The driver state; unused by this rule.
		 * @param HandleOwnerProcess The process receiving the handle.
		 * @param TargetProcess The protected process referenced by the handle.
		 * @param Information The pending operation and modifiable requested access.
		 */
		static void ApplyProtectedProcessRule(FDriverContext& Context, PEPROCESS HandleOwnerProcess, PEPROCESS TargetProcess, OB_PRE_OPERATION_INFORMATION& Information) noexcept;

		/**
		 * @brief Restricts a process handle received by an isolated process when enabled.
		 * @param Context The driver state containing isolation flags.
		 * @param HandleOwnerProcess The isolated process receiving the handle.
		 * @param TargetProcess The process referenced by the handle.
		 * @param Information The pending operation and modifiable requested access.
		 */
		static void ApplyIsolationProcessRule(FDriverContext& Context, PEPROCESS HandleOwnerProcess, PEPROCESS TargetProcess, OB_PRE_OPERATION_INFORMATION& Information) noexcept;
	};

	class FPreOperationThreadRules
	{
	public:
		/**
		 * @brief Classifies a handle targeting a thread and dispatches matching rules.
		 *
		 * Thread-right restrictions are not implemented yet.
		 * @param Context The driver state containing Agent and process registries.
		 * @param HandleOwnerProcess The process receiving the handle, including a duplication destination.
		 * @param TargetThread The thread referenced by the handle, not the requesting thread.
		 * @param Information The pending operation and modifiable requested access.
		 */
		static void ApplyRule(FDriverContext& Context, PEPROCESS HandleOwnerProcess, PETHREAD TargetThread, OB_PRE_OPERATION_INFORMATION& Information) noexcept;

		/**
		 * @brief Removes selected rights from handles to a protected process's thread.
		 * @param Context The driver state; unused by this rule.
		 * @param HandleOwnerProcess The process receiving the handle.
		 * @param TargetProcess The process that owns the target thread.
		 * @param TargetThread The protected process's thread referenced by the handle.
		 * @param Information The pending operation and modifiable requested access.
		 */
		static void ApplyProtectedThreadRule(FDriverContext& Context, PEPROCESS HandleOwnerProcess, PEPROCESS TargetProcess, PETHREAD TargetThread, OB_PRE_OPERATION_INFORMATION& Information) noexcept;
	};
};
