#pragma once

#include "Core/Public/Misc/Types.h"

namespace Argus::Notify
{
	inline constexpr uint16 ProcessNotifyMaxCommandLineBytes = 256;
	inline constexpr uint16 ProcessNotifyMaxImageFileNameBytes = 512;

	enum class EProcessNotifyType : uint8
	{
		Unknown,
		Created,
		Exited,
		
		CreationDenied,
		CreationFailed,
	};
}
