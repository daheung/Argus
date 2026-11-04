#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"

class FDriverContext;

namespace Driver
{
	NTSTATUS ArgusDeviceControl(PDEVICE_OBJECT DeviceObject, PIRP Irp);
}

NTSTATUS DeviceControlGetProcessNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DeviceControlGetThreadNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DeviceControlGetImageNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DeviceControlGetRegistryNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DeviceControlGetObjectOperationNotify(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DeviceControlRegisterAgent(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DeviceControlRegisterProcess(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp);

NTSTATUS DeviceControlRegisterIsolationProcess(FDriverContext* Context, PDEVICE_OBJECT DeviceObject, PIRP Irp);