#pragma once

#include "Core/Public/Misc/Types.h"

/*
 * 1. 작업을 요청한 Process가 어떤 Role을 적용받는지
 * 2. Role에 대해 어떤 제약을 걸건지
 *  - AgentProcess
 *  - IsolatedProcess
 *  - ProtectedProcess
 * 3. 작업 결과가 Argus가 적용한 것인지 아닌지
 */

namespace Argus::Policy
{
	enum class EPolicyRole
	{
		None,
		AgentProcess,
		IsolatedProcess,
		ProtectedProcess,
	};

	enum class EPolicyStatus : uint8
	{
		AccessNone,
		AccessPass,
		AccessAllowed,
		AccessBlocked,
		AccessReduced,
	};

	enum class EIsolationPolicyFlags : uint32
	{
		None					  = 0u,
		BlockChildProcessCreation = 1u << 0,
		ReduceProcessHandleRights = 1u << 1,
		ReduceThreadHandleRights  = 1u << 2,
	};

	enum class EProtectionPolicyFlags : uint32
	{
		ProtectNone				= 0u, 
		ProtectProcessHandle	= 1u << 0,
		ProtectThreadHandle		= 1u << 1,
	};
}