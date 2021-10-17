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
// @brief ���������� ��������� ��������
// @details The device context performs the same job
//  as a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    WDFDEVICE               Device;

    WDFINTERRUPT            Interrupt;          //! ��������� ����������� ����������
    //WDFSPINLOCK             IsrLock;          //! ������ ������������� ��� ����������

    // ��������� IOCTL
    WDFQUEUE                cntrlQueue;         //! ������� ��������� IOCtl �������� �� ���������
    WDFQUEUE                asyncQueue;         //! ������� ��� ��������� ������ �������

    // ���������� ��� ������ ������� ��������
    ULONG                   HwErrCount;
    ULONG                   InterruptCount;     //! ������� ������������ ����������

    ULONG                   inpOutNgVersion;    //! ����� ������ ��������

    ULONG                   irqClearSize;       //! ������ ������� ��� ������� ����������
    port_task_t             irqClearSeq[16];    //! ��������� ������� ��� �������
    port_task_t             irqClearAck[16];    //! ��������� ������������� ���������� �������

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
    _In_ PWDFDEVICE_INIT DeviceInit,
    _Out_ WDFDEVICE      *Device
    );

NTSTATUS
inpOutNgPrepareHardware(
    _In_ PINPOUTNG_CONTEXT devContext,
    _In_ WDFCMRESLIST    ResourcesTranslated
);

NTSTATUS
inpOutNgInitializeDeviceContext(
    _In_ PINPOUTNG_CONTEXT devContext
);

VOID
inpOutNgCleanupDeviceContext(
    _In_ PINPOUTNG_CONTEXT devContext
);

NTSTATUS
inpOutNgInitWrite(
    _In_ PINPOUTNG_CONTEXT devContext
);

NTSTATUS
inpOutNgInitRead(
    _In_ PINPOUTNG_CONTEXT devContext
);

VOID
inpOutNgShutdown(
    _In_ PINPOUTNG_CONTEXT devContext
);

VOID
inpOutNgHardwareReset(
    _In_ PINPOUTNG_CONTEXT devContext
);

VOID
doTask(
    _In_ PINPOUTNG_CONTEXT devContext,
    _In_ p_port_task_t taskItemIn,
    _Out_ p_port_task_t taskItemOut
);

EXTERN_C_END
