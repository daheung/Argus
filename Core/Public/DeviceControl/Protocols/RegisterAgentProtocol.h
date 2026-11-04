#pragma once

#include "Core/Public/Misc/Types.h"

namespace Argus::DeviceControl
{
	struct FRegisterAgent
	{
		uint32 MajorVersion;
		uint32 MinorVersion;
		uint32 PatchVersion;
	};
}