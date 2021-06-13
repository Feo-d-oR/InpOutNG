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

    WDFINTERRUPT            Interrupt;      // Ёкземпл€р обработчика прерываний

    // IOCTL handling
    WDFQUEUE                cntrlQueue;     // ќчередь обработки IOCtl запросов по умолчанию
    WDFQUEUE                asyncQueue;     // ќчередь дл€ инверсной модели вызовов

    ULONG                   HwErrCount;
    ULONG                   InterruptCount; // —чЄтчик обработанных прерываний

    ULONG                   inpOutNgVersion;// Ќомер версии драйвера
    BOOLEAN                 ReadReady;
    BOOLEAN                 WriteReady;

} INPOUTNG_CONTEXT, *PINPOUTNG_CONTEXT;

//
// This macro will generate an inline function called inpOutNgGetContext
// which will be used to get a pointer to the device context memory
// in a type safe manner.
//
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(INPOUTNG_CONTEXT, inpOutNgGetContext)

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
    IN PINPOUTNG_CONTEXT DevContext,
    IN WDFCMRESLIST    ResourcesTranslated
);

NTSTATUS
inpOutNgInitializeDeviceContext(
    IN PINPOUTNG_CONTEXT DevContext
);

VOID
inpOutNgCleanupDeviceContext(
    IN PINPOUTNG_CONTEXT DevContext
);

NTSTATUS
inpOutNgInitWrite(
    IN PINPOUTNG_CONTEXT DevContext
);

NTSTATUS
inpOutNgInitRead(
    IN PINPOUTNG_CONTEXT DevContext
);

VOID
inpOutNgShutdown(
    IN PINPOUTNG_CONTEXT DevContext
);

VOID
inpOutNgHardwareReset(
    IN PINPOUTNG_CONTEXT DevContext
);

EXTERN_C_END
