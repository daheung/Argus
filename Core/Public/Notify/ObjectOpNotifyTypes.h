#pragma once

#include "Core/Public/Misc/Types.h"

namespace Argus::Notify
{
	enum class EObjectOperationType : uint32
	{
		Unknown,
		Create,
		Duplicate
	};

	enum class EObjectTargetType : uint32
	{
		Unknown,
		Process,
		Thread
	};

	enum class EObjectTargetRole : uint8
	{
		None	  = 0u,
		Agent	  = 1u << 0,
		Protected = 1u << 1,
		Isolated  = 1u << 2,
	};
}
