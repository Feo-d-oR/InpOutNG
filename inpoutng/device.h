/*++

Module Name:

    device.h

Abstract:

    This file contains the device definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include "public.h"

EXTERN_C_START

//
// The device context performs the same job as
// a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    WDFDEVICE               Device;

    // Following fields are specific to the hardware
    // Configuration

    WDFINTERRUPT            Interrupt;     // Returned by InterruptCreate

    // IOCTL handling
    WDFQUEUE                ControlQueue;
    BOOLEAN                 RequireSingleTransfer;

    ULONG                   HwErrCount;
    BOOLEAN                 ReadReady;
    BOOLEAN                 WriteReady;

} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

//
// This macro will generate an inline function called inpOutNgGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, inpOutNgGetContext)

//
// Function to initialize the device and its callbacks
//
NTSTATUS
inpOutNgCreateDevice(
    IN PWDFDEVICE_INIT DeviceInit,
    OUT WDFDEVICE      *Device
    );

NTSTATUS
inpOutNgPrepareHardware(
    IN PDEVICE_CONTEXT DevExt,
    IN WDFCMRESLIST    ResourcesTranslated
);

NTSTATUS
inpOutNgInitializeDeviceContext(
    IN PDEVICE_CONTEXT DevExt
);

VOID
inpOutNgCleanupDeviceContext(
    IN PDEVICE_CONTEXT DevExt
);

NTSTATUS
inpOutNgInitWrite(
    IN PDEVICE_CONTEXT DevExt
);

NTSTATUS
inpOutNgInitRead(
    IN PDEVICE_CONTEXT DevExt
);

VOID
inpOutNgShutdown(
    IN PDEVICE_CONTEXT DevExt
);

VOID
inpOutNgHardwareReset(
    IN PDEVICE_CONTEXT DevExt
);

EXTERN_C_END
