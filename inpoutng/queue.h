/*++

Module Name:

    queue.h

Abstract:

    This file contains the queue definitions.

Environment:

    Kernel-mode Driver Framework

--*/

EXTERN_C_START

//
// This is the context that can be placed per queue
// and would contain per queue information.
//
typedef struct _QUEUE_CONTEXT {

    ULONG PrivateDeviceData;  // just a placeholder

} QUEUE_CONTEXT, *PQUEUE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(QUEUE_CONTEXT, QueueGetContext)

/**
 * @brief »нициализаци€ очередей обработки запросов
 * @param Device экземпл€р устройства, обслуживаемого драйвером
 * @return \li STATUS_SUCCESS
 *         \li
 * @detail The I/O dispatch callbacks for the frameworks device object
 *         are configured in this function.
 *
 *    A single default I/O Queue is configured for parallel request
 *    processing, and a driver context memory allocation is created
 *    to hold our structure QUEUE_CONTEXT.
*/
NTSTATUS
inpOutNgQueueInitialize(
    _In_ WDFDEVICE Device
    );

//
// Events from the IoQueue object
//
/**
 * @brief ‘ункци€ обработки вход€щих запросов на операции ввода/вывода
 * @param Queue Ч ƒескриптор обработчика очереди поступающих запросов
 * @param Request Ч ƒескриптор запроса
 * @param OutputBufferLength Ч –азмер выходного буфера в байтах
 * @param InputBufferLength Ч –азмер входного буфера в байтах
 * @param IoControlCode Ч  од операции ввода/вывода
*/
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL inpOutNgEvtIoDeviceControl;

/**
 * @brief This event is invoked for a power-managed queue before the device leaves the working state (D0).
 * @param Queue Ч  ƒескриптор обработчика очереди поступающих запросов
 * @param Request Ч ƒескриптор запроса
 * @param ActionFlags Ч A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
 *                      that identify the reason that the callback function is being called
 *                      and whether the request is cancelable.
 * @detail
 *         In most cases, the EvtIoStop callback function completes, cancels, or postpones
 *         further processing of the I/O request.
 *
 *         Typically, the driver uses the following rules:
 *
 *         \li If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
 *           (if the request is cancelable) and either calls WdfRequestStopAcknowledge
 *           with a Requeue value of TRUE, or it calls WdfRequestComplete with a
 *           completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
 *
 *           Before it can call these methods safely, the driver must make sure that
 *           its implementation of EvtIoStop has exclusive access to the request.
 *
 *           In order to do that, the driver must synchronize access to the request
 *           to prevent other threads from manipulating the request concurrently.
 *           The synchronization method you choose will depend on your driver's design.
 *
 *           For example, if the request is held in a shared context, the EvtIoStop callback
 *           might acquire an internal driver lock, take the request from the shared context,
 *           and then release the lock. At this point, the EvtIoStop callback owns the request
 *           and can safely complete or requeue the request.
 *
 *         \li If the driver has forwarded the I/O request to an I/O target, it either calls
 *           WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
 *           further processing of the request and calls WdfRequestStopAcknowledge with
 *           a Requeue value of FALSE.
 *
 *         A driver might choose to take no action in EvtIoStop for requests that are
 *         guaranteed to complete in a small amount of time.
 *
 *         In this case, the framework waits until the specified request is complete
 *         before moving the device (or system) to a lower power state or removing the device.
 *         Potentially, this inaction can prevent a system from entering its hibernation state
 *         or another low system power state. In extreme cases, it can cause the system
 *         to crash with bugcheck code 9F.
 *
 *
*/
EVT_WDF_IO_QUEUE_IO_STOP inpOutNgEvtIoStop;

/**
 * @brief ‘ункци€ обработчика очереди асинхронных уведомлений
 * @param devContext Ч ƒескриптор данных устройства
*/
VOID
inpOutNgNotify(
    _In_ PINPOUTNG_CONTEXT devContext
);
EXTERN_C_END
