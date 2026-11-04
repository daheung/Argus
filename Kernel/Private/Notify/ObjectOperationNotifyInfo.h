#pragma once

#include "Kernel/Public/Misc/KernelDefines.h"
#include "Core/Public/Notify/ObjectOpNotifyTypes.h"

namespace Argus::Notify
{
    struct FObjectOperationNotifyInfo
    {
        LARGE_INTEGER PreOperationAt{};
        LARGE_INTEGER PostOperationAt{};

        EObjectOperationType OperationType = EObjectOperationType::Unknown;
        EObjectTargetType TargetType = EObjectTargetType::Unknown;

        uint32 RequesterProcessId = 0;
        uint32 RequesterThreadId = 0;
        uint32 HandleOwnerProcessId = 0;

        // EObjectOperationType::Duplicate
        uint32 SourceProcessId = 0;  
        uint32 TargetProcessId = 0;

        // EObjectTargetType::Thread
        uint32 TargetThreadId = 0;

        ACCESS_MASK OriginalDesiredAccess = 0;
        ACCESS_MASK DesiredAccessBeforePolicy = 0;
        ACCESS_MASK DesiredAccessAfterPolicy = 0;
        ACCESS_MASK GrantedAccess = 0;

		NTSTATUS ReturnStatus = STATUS_UNSUCCESSFUL;
    };
}
