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

EVT_WDF_OBJECT_CONTEXT_CLEANUP inpOutNgEvtDeviceCleanup;
EVT_WDF_DEVICE_D0_ENTRY inpOutNgEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT inpOutNgEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE inpOutNgEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE inpOutNgEvtDeviceReleaseHardware;

/*
EVT_WDF_IO_QUEUE_IO_READ inpOutNgEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE inpOutNgEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL inpOutNgEvtIoDeviceControl;
*/

EVT_WDF_INTERRUPT_ISR inpOutNgEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC inpOutNgEvtInterruptDpc;
EVT_WDF_INTERRUPT_ENABLE inpOutNgEvtInterruptEnable;
EVT_WDF_INTERRUPT_DISABLE inpOutNgEvtInterruptDisable;


NTSTATUS
inpOutNgInterruptCreate(
    IN PDEVICE_CONTEXT DevExt
);

NTSTATUS
inpOutNgSetIdleAndWakeSettings(
    IN PDEVICE_CONTEXT FdoData
);

EXTERN_C_END
