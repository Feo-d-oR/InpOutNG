/*! \mainpage Input Output Next-Gen (InpOutNG) driver
 *
 * \section intro_sec Introduction
 *
 * This is the introduction.
 *
 * \section install_sec Installation
 *
 * \subsection step1 Step 1: Opening the box
 *
 * etc...
 */
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

EVT_WDF_INTERRUPT_ISR inpOutNgEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC inpOutNgEvtInterruptDpc;
EVT_WDF_INTERRUPT_ENABLE inpOutNgEvtInterruptEnable;
EVT_WDF_INTERRUPT_DISABLE inpOutNgEvtInterruptDisable;
EVT_WDF_TIMER inpOutNgOnTimer;

NTSTATUS
inpOutNgInterruptCreate(
    _In_ PINPOUTNG_CONTEXT devContext
);

NTSTATUS
inpOutNgSetIdleAndWakeSettings(
    _In_ PINPOUTNG_CONTEXT devContext
);

EXTERN_C_END
