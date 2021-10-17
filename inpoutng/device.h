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
// @brief Внутренние параметры драйвера
// @details The device context performs the same job
//  as a WDM device extension in the driver frameworks
//
typedef struct _DEVICE_CONTEXT
{
    WDFDEVICE               Device;

    WDFINTERRUPT            Interrupt;          //! Экземпляр обработчика прерываний
    //WDFSPINLOCK             IsrLock;          //! Объект синхронизации для прерываний

    // Обработка IOCTL
    WDFQUEUE                cntrlQueue;         //! Очередь обработки IOCtl запросов по умолчанию
    WDFQUEUE                asyncQueue;         //! Очередь для инверсной модели вызовов

    // Переменные для работы функций драйвера
    ULONG                   HwErrCount;
    ULONG                   InterruptCount;     //! Счётчик обработанных прерываний

    ULONG                   inpOutNgVersion;    //! Номер версии драйвера

    ULONG                   irqClearSize;       //! Размер задания для очистки прерывания
    port_task_t             irqClearSeq[16];    //! Структура задания для очистки
    port_task_t             irqClearAck[16];    //! Структура подтверждения выполнения очистки

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
