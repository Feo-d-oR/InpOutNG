/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

#include "Device.h"
#include "Queue.h"
#include "Trace.h"

EXTERN_C_START

NTSTATUS UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, PVOID pPhysMemLin);
NTSTATUS MapPhysicalMemoryToLinearSpace(PVOID pPhysAddress,
    SIZE_T PhysMemSizeInBytes,
    PVOID* ppPhysMemLin,
    HANDLE* pPhysicalMemoryHandle);


//extern struct tagPhys32Struct Phys32Struct;
//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD inpoutngEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP inpoutngEvtDriverContextCleanup;

EXTERN_C_END
