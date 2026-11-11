#pragma once

#include "Core/Public/Misc/Types.h"

#if defined(PLATFORM_WDM) || defined(PLATFORM_KMDF)
	#include <ntifs.h>	
#else
	#include <Windows.h>
	#include <winioctl.h>
#endif


namespace Argus::DeviceControl
{
	inline constexpr uint32 IoctlCodeGetProcessNotify 
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeGetThreadNotify
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeGetImageNotify
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeGetRegistryNotify
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeGetObjectOperationNotify
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS);


	inline constexpr uint32 IoctlCodeRegisterProcess
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x810, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeUnregisterProcess
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x811, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeRegisterAgent 
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x812, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeRegisterIsocationProcess
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x813, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeUnregisterIsolcationProcess
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x814, METHOD_BUFFERED, FILE_READ_ACCESS);


	inline constexpr uint32 IoctlCodeFltRegisterProcess
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x820, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeFltUnregisterProcess
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x821, METHOD_BUFFERED, FILE_READ_ACCESS);

	inline constexpr uint32 IoctlCodeFltGetFileOperations
		= CTL_CODE(FILE_DEVICE_UNKNOWN, 0x822, METHOD_OUT_DIRECT, FILE_READ_ACCESS);
}