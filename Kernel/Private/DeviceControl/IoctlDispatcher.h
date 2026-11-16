#pragma once

#include "Kernel/Public/Misc/KernelTypes.h"

template <typename Protocol>
struct TIoctlProtocolDescription;

template <uint32 IoctlCode>
class FIoctlDispatcher
{
public:
	template <typename Protocol>
	static NTSTATUS Dispatch(TIoctlProtocolDescription<Protocol> Description) noexcept;
};
