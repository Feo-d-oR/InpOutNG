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

#include "device.h"
#include "queue.h"
#include "trace.h"

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
EVT_WDF_DRIVER_DEVICE_ADD inpOutNgEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP inpOutNgEvtDriverContextCleanup;

NTSTATUS
inpOutNgInterruptCreate(
    IN PDEVICE_CONTEXT DevExt
);

BOOLEAN
inpOutNgEvtInterruptIsr(
    IN WDFINTERRUPT Interrupt,
    IN ULONG        MessageID
);

VOID
inpOutNgEvtInterruptDpc(
    WDFINTERRUPT Interrupt,
    WDFOBJECT    Device
);

NTSTATUS
inpOutNgEvtInterruptEnable(
    IN WDFINTERRUPT Interrupt,
    IN WDFDEVICE    Device
);

NTSTATUS
inpOutNgEvtInterruptDisable(
    IN WDFINTERRUPT Interrupt,
    IN WDFDEVICE    Device
);



EXTERN_C_END
