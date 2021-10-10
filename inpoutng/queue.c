/*++

Module Name:

    queue.c

Abstract:

    This file contains the queue entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "queue.tmh"

/**
* 
*/

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, inpOutNgQueueInitialize)
#endif
/**
 * @brief Инициализация очередей обработки запросов
 * @param Device экземпляр устройства, обслуживаемого драйвером
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
    )
{
    NTSTATUS            status;
    PINPOUTNG_CONTEXT   devContext;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    devContext = inpOutNgGetContext(Device);
    
    //
    // Создание очереди событий «по умолчанию», в которой обрабатываются запросы,
    // не сконфигурированные на диспетчеризацию через WdfDeviceConfigureRequestDispatching
    // к следованию через прочие очереди.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchParallel
        );

    
    //
    // Declare our I/O Event Processing callbacks
    //
    // This driver only handles IOCTLs.
    //
    // WDF will automagically handle Create and Close requests for us and will
    // will complete any OTHER request types with STATUS_INVALID_DEVICE_REQUEST.    
    //
    queueConfig.EvtIoDeviceControl = inpOutNgEvtIoDeviceControl;

    //
    // Because this is a queue for a non-powermanaged legacy devices, indicate
    // that the queue doesn't need to be power managed
    //
    queueConfig.PowerManaged = WdfFalse;
    
    //queueConfig.EvtIoStop = inpOutNgEvtIoStop;

    status = WdfIoQueueCreate(
        Device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        WDF_NO_HANDLE
        );

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfDefaultQueueCreate failed %!STATUS!", status);
        return status;
    }

    //
    // Создание очереди для обработки запросов на асинхронное уведомление.
    // Метод асинхронных уведомлений через инверсные функции обратного вызова
    // (Inverted call model) описана в статье OSR по адресу
    // https://www.osr.com/nt-insider/2013-issue1/inverted-call-model-kmdf/
    // 
    // В данную очередь будут помещаться запросы, завершающиеся по возникновению события
    // (прерывания или настраиваемого)
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
        WdfIoQueueDispatchManual);

    queueConfig.PowerManaged = WdfFalse;
    queueConfig.EvtIoStop = inpOutNgEvtIoStop;
    queueConfig.AllowZeroLengthRequests = WdfTrue;

    status = WdfIoQueueCreate(devContext->Device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->asyncQueue);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE,
            "WdfAsyncQueueCreate failed: %!STATUS!", status);
        return status;
    }

    return status;
}

static __inline ULONG checkInBuffer(_In_ size_t inSz)
{
    return (inSz >= sizeof(inPortData_t)) ? 1 : 0;
}

static __inline ULONG checkOutBuffer(_In_ size_t outSz)
{
    return (outSz >= sizeof(outPortData_t)) ? 1 : 0;
}

static __inline ULONG checkIOBuffers(_In_ size_t inSz, _In_ size_t outSz)
{
    return (checkInBuffer(inSz) && checkOutBuffer(outSz)) ? 1 : 0;
}

/**
 * @brief Функция обработки входящих запросов на операции ввода/вывода
 * @param Queue — Дескриптор обработчика очереди поступающих запросов
 * @param Request — Дескриптор запроса
 * @param OutputBufferLength — Размер выходного буфера в байтах
 * @param InputBufferLength — Размер входного буфера в байтах
 * @param IoControlCode — Код операции ввода/вывода
*/
VOID
inpOutNgEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
{
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Req 0x%p OBufLen %d IBufLen %d IoCtlCode %08x", 
                Queue, Request, (int) OutputBufferLength, (int) InputBufferLength, IoControlCode);
    
    NTSTATUS            status = STATUS_UNSUCCESSFUL;
    size_t              opInfo = 0;
    size_t              inBuffersize = 0;
    size_t              outBuffersize = 0;

    p_inPortData_t      inData = NULL;
    outPortData_t       outData = { .val.outLong = 0x0 };

    PULONG              inBuf = NULL;
    PULONG              outBuf = NULL;
    PINPOUTNG_CONTEXT   devContext;

    // Получение ссылки на контекст устройства через идентификатор очереди...
    devContext = inpOutNgGetContext(WdfIoQueueGetDevice(Queue));

    // Если размер входного буфера ноль и это не запрос версии или прерывания, здесь делать нечего
    if (!InputBufferLength && (IoControlCode != IOCTL_GET_DRVINFO) && (IoControlCode != IOCTL_REGISTER_IRQ))
    {
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }
    
    if (InputBufferLength != 0) {
        //
        // For buffered ioctls WdfRequestRetrieveInputBuffer &
        // WdfRequestRetrieveOutputBuffer return the same buffer
        // pointer (Irp->AssociatedIrp.SystemBuffer), so read the
        // content of the buffer.
        //
        status = WdfRequestRetrieveInputBuffer(Request, 0, &inBuf, &inBuffersize);
        if (!NT_SUCCESS(status)) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }

        // Проверить, что система правильно посчитала размер входного буфера
        ASSERT(inBuffersize >= InputBufferLength);

        // И указатель на него не нулевой
        ASSERT(inBuf != NULL);

        // «Сослать» на входной буфер типизированный указатель для последующего разбора */
        inData = (p_inPortData_t)inBuf;

        //TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Data from User : (0x%p) 0x%08x\n", inBuf, *inBuf);

        // 
        // Если длина выходного буфера не ноль, требуется получить указатель на него и проверить,
        // что система определила размер корректно и указатель не нулевой
        //
        if (OutputBufferLength != 0)
        {
            status = WdfRequestRetrieveOutputBuffer(Request, 0, &outBuf, &outBuffersize);
            if (!NT_SUCCESS(status)) {
                status = STATUS_INSUFFICIENT_RESOURCES;
            }
            ASSERT(outBuffersize >= OutputBufferLength);
            ASSERT(outBuf != NULL);
            //TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Data to User : (0x%p)\n", outBuf);
        }
    }

    switch (IoControlCode)
    {
        case IOCTL_READ_PORT_UCHAR: {
            if (!checkIOBuffers(inBuffersize, outBuffersize))
            {
                opInfo = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                outData.val.outChar = __inbyte(inData->addr);
                opInfo = sizeof(UCHAR);
                status = STATUS_SUCCESS;
            }
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Done IOCTL_READ_PORT_UCHAR (P=0x%04x, V=0x%02x), Status=0x%08x, Return=%d\n",
                inData->addr, outData.val.outChar, status, (UINT32)opInfo);
            break;
        }

        case IOCTL_WRITE_PORT_UCHAR: {
            if (!checkInBuffer(inBuffersize))
            {
                opInfo = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                __outbyte(inData->addr, inData->val.inChar);	//Byte 0,1=Address Byte 2=Value
                opInfo = 10;
                status = STATUS_SUCCESS;
            }
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Done IOCTL_WRITE_PORT_UCHAR (P=0x%04x, V=0x%02x), Status=0x%08x, Return=%d\n",
                inData->addr, inData->val.inChar, status, (UINT32)opInfo);
            break;
        }
               
        case IOCTL_READ_PORT_USHORT: {
            if (!checkIOBuffers(inBuffersize, outBuffersize))
            {
                opInfo = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                outData.val.outShrt = __inword(inData->addr);
                opInfo = sizeof(USHORT);
                status = STATUS_SUCCESS;
            }
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Done IOCTL_READ_PORT_USHORT (P=0x%04x, V=0x%04x), Status=0x%08x, Return=%d\n",
                inData->addr, outData.val.outShrt, status, (UINT32)opInfo);
            break;
        }

        case IOCTL_WRITE_PORT_USHORT: {
            if (!checkInBuffer(inBuffersize))
            {
                opInfo = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                __outword(inData->addr, inData->val.inShrt); //Short 0=Address Short 1=Value
                opInfo = 10;
                status = STATUS_SUCCESS;
            }
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Done IOCTL_WRITE_PORT_USHORT (P=0x%04x, V=0x%04x), Status=0x%08x, Return=%d\n",
                inData->addr, inData->val.inShrt, status, (UINT32)opInfo);
            break;
        }

        case IOCTL_READ_PORT_ULONG: {
            if (!checkIOBuffers(inBuffersize, outBuffersize))
            {
                opInfo = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                outData.val.outLong = __indword(inData->addr);
                opInfo = sizeof(ULONG);
                status = STATUS_SUCCESS;
            }
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Done IOCTL_READ_PORT_ULONG (P=0x%04x, V=0x%08x), Status=0x%08x, Return=%d\n",
                inData->addr, outData.val.outLong, status, (UINT32)opInfo);
            break;
        }

        case IOCTL_WRITE_PORT_ULONG: {
            if (!checkInBuffer(inBuffersize))
            {
                opInfo = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else
            {
                __outdword(inData->addr, inData->val.inLong); //Short 0=Address long 1=Value
                opInfo = 10;
                status = STATUS_SUCCESS;
            }
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Done IOCTL_WRITE_PORT_ULONG (P=0x%04x, V=0x%08x), Status=0x%08x, Return=%d\n",
                inData->addr, inData->val.inLong, status, (UINT32)opInfo);
            break;
        }

        case IOCTL_REGISTER_IRQ: {

            //
            // Возвращаемые значения кратны типу элемента задания,
            // необходимо проверить, что выходной буфер кратен данному значению
            // перед перенаправлением запроса в очередь ожидания.
            // 
            if ( InputBufferLength != 0) {
                if ((((int)inBuffersize / sizeof(port_task_t)) > 0)
                    &&
                    (!(outBuffersize >= sizeof(port_task_t)))) {

                    opInfo = 0;
                    status = STATUS_BUFFER_TOO_SMALL;
                    break;
                }
            }
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC,
                        "%!FUNC! ---> IOCTL_REGISTER_IRQ, (ptrin==%p, listin=%d, ptrout==%p, listout==%d)\n",
                        inBuf, (int)inBuffersize / sizeof(port_task_t), outBuf, (int)outBuffersize / sizeof(port_task_t));
            
            status = WdfRequestForwardToIoQueue(Request,
                devContext->asyncQueue);
            
            //
            // Если запрос не может быть перенаправлен в очередь ожидания,
            // требуется его завершить. Статус операции будет соответствовать коду ошибки
            // перенаправления в очередь (WdfRequestForwardToIoQueue).
            // 
            if (!NT_SUCCESS(status)) {
                opInfo = 0;
                TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! Error forwarding to queue! status==0x%x\n", status);
                break;
            }
            
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! <--- IOCTL_REGISTER_IRQ\n");
            //
            // *** ЗАПРОС ПОСТАВЛЕН В ОЖИДАНИЕ ***
            //     Статус операции будет выставлен по его завершению,
            //     выход из данного обработчика.
            //
            return;
        }

        case IOCTL_TEST_ASYNC: {
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! ---> IOCTL_IOCTL_TEST_ASYNC\n");
            inpOutNgNotify(devContext);
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! <--- IOCTL_IOCTL_TEST_ASYNC\n");
            break;
        }

        case IOCTL_GET_DRVINFO: {
            if (!(outBuffersize >= sizeof(outPortData_t))) {
                opInfo = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else {
                outData.val.outLong = devContext->inpOutNgVersion;
                opInfo = sizeof(ULONG);
                status = STATUS_SUCCESS;
            }
            break;
        }

        case IOCTL_GET_IRQCOUNT: {
            if (!(outBuffersize >= sizeof(outPortData_t))) {
                opInfo = 0;
                status = STATUS_BUFFER_TOO_SMALL;
            }
            else {
                outData.val.outLong = devContext->InterruptCount;
                opInfo = sizeof(ULONG);
                status = STATUS_SUCCESS;
            }
            break;
        }

        default: {
            opInfo = 0;
            status = STATUS_UNSUCCESSFUL;
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Unknown IOCTL 0x%08x, Status=0x%08x, Return=%d\n", IoControlCode, status, (UINT32)opInfo);
            break;
        }

    }
    
    if ((opInfo > 0) && (outBuffersize >= opInfo))
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Committing data to User: (P=0x%p, S=%d bytes), Status=0x%x, opSize=%d\n",
            outBuf, (int)outBuffersize, status, (UINT32)opInfo);
        RtlCopyMemory(outBuf, &outData, opInfo);
    }

    //TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Calling completion for main IOCtl handler...\n");

    WdfRequestCompleteWithInformation(Request,
        status,
        opInfo);

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! <--- Exiting... Status=0x%x, opInfo=%d\n", status, (UINT32)opInfo);

    return;
}

/**
 * @brief This event is invoked for a power-managed queue before the device leaves the working state (D0).
 * @param Queue —  Дескриптор обработчика очереди поступающих запросов
 * @param Request — Дескриптор запроса
 * @param ActionFlags — A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
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
 *           with a Requeue value of WdfTrue, or it calls WdfRequestComplete with a
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
VOID
inpOutNgEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
{
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
                Queue, Request, ActionFlags);

    return;
}

/**
 * @brief Функция обработчика очереди асинхронных уведомлений
 * @param DevContext — Дескриптор данных устройства
*/
VOID
inpOutNgNotify(PINPOUTNG_CONTEXT DevContext)
{
    NTSTATUS      status=STATUS_ABANDONED;
    ULONG         opInfo=0;
    WDFREQUEST    notifyRequest;

    PULONG        inBuf = NULL;
    size_t        inBufferSize=0;
    p_port_task_t inTask=NULL;

    PULONG        outBuf = NULL;
    size_t        outBufferSize=0;
    p_port_task_t outTask=NULL;

    size_t        taskSize=0;
    INT           i=0;

    //TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! ---> Got in\n");

    status = WdfIoQueueRetrieveNextRequest(
        DevContext->asyncQueue,
        &notifyRequest);

    //
    // Проверка на наличие ожидающих запросов в очереди
    // 
    if (!NT_SUCCESS(status)) {

        //
        // Запросов нет. Успешно забрать запрос из очереди ожидания не вышло.
        // Возможно, в очереди попросту нет запросов. В любом случае — на выход
        // 
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! ---> No queries. Exiting.");
        return;
    }

    //
    // Обработка полученного из очереди запроса.
    // 

    //
    // Получение указателей на буферы приёма / передачи информации из приложения.
    // Аналогично с «общей» очередь.
    // 

    status = WdfRequestRetrieveInputBuffer(notifyRequest, 0, &inBuf, &inBufferSize);
    //
    // Проверка на наличие входного буфера
    // 
    if (!NT_SUCCESS(status)) {

        //
        // Буфер с заданием пуст, завершить запрос со статусом — нет ресурсов.
        // 
        status = STATUS_INSUFFICIENT_RESOURCES;
        opInfo = 0;
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! ---- No input task buffer! Exiting.");
        goto queue_completion;
    }

    status = STATUS_INSUFFICIENT_RESOURCES;
    opInfo = 0;
    
    if (inBufferSize > 0) {
        // Проверить, что размер входного буфера больше 32-х разрядов
        if (inBufferSize < sizeof(port_task_t)) {
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! ---- Input task buffer too small (%d bytes)! Exiting.\n", (int)inBufferSize);
            goto queue_completion;
        }

        // Проверить, что указатель на буфер не нулевой
        if (inBuf == NULL) {
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! ---- Input task buffer pointer is NULL! Exiting.\n");
            goto queue_completion;
        }

        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! Data from User : 0x%p, %d bytes\n", inBuf, (int)inBufferSize);
    }
    else {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! Zero-sized request, notify only\n");
    }
    //
    // We successfully retrieved a Request from the notification Queue
    // AND we retrieved an output buffer into which to return some
    // additional information.
    // 
    status = WdfRequestRetrieveOutputBuffer(notifyRequest, 0, &outBuf, &outBufferSize);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! ---- No output task buffer found! Exiting.\n");
        goto queue_completion;
    }

    if (inBufferSize > 0) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        opInfo = 0;
        if (outBufferSize != inBufferSize) {
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC,
                "%!FUNC! ---- Input / Output buffers size mismatch (%d != %d)! Exiting.\n",
                (int)inBufferSize, (int)outBufferSize);
            goto queue_completion;
        }

        if (outBuf == NULL) {
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! ---- Output task buffer pointer is NULL! Exiting.\n");
            goto queue_completion;
        }

        inTask = (p_port_task_t)inBuf;
        outTask = (p_port_task_t)outBuf;
    }
    else {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! Zero-sized request, not dealing with output\n");
    }

    taskSize = inBufferSize / sizeof(port_task_t);

    //TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! ---- Got %d tasks in list, proceeding...\n", (int)taskSize);

    for (i = 0; i < (INT)taskSize; i++)
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC,
                    "%!FUNC! ----> parsing task: code==0x%04x, port==0x%04x, data==0x%08x\n",
                    inTask[i].taskOperation, inTask[i].taskPort, inTask[i].taskData);
        switch (inTask[i].taskOperation) {
            case INPOUT_READ8: {
                outTask[i].taskOperation = INPOUT_READ8_ACK;
                outTask[i].taskData = __inbyte(inTask[i].taskPort);
                break;
            }
            case INPOUT_WRITE8: {
                outTask[i].taskOperation = INPOUT_WRITE8_ACK;
                __outbyte(inTask[i].taskPort, (BYTE)(inTask[i].taskData & 0x000000ff));
                outTask[i].taskData = 0x0;
                break;
            }
            case INPOUT_READ16: {
                outTask[i].taskOperation = INPOUT_READ16_ACK;
                outTask[i].taskData = __inword(inTask[i].taskPort);
                break;
            }
            case INPOUT_WRITE16: {
                outTask[i].taskOperation = INPOUT_WRITE16_ACK;
                __outword(inTask[i].taskPort, (USHORT)(inTask[i].taskData & 0x0000ffff));
                outTask[i].taskData = 0x0;
                break;
            }
            case INPOUT_READ32: {
                outTask[i].taskOperation = INPOUT_READ32_ACK;
                outTask[i].taskData = __indword(inTask[i].taskPort);
                break;
            }
            case INPOUT_WRITE32: {
                outTask[i].taskOperation = INPOUT_WRITE32_ACK;
                __outdword(inTask[i].taskPort, inTask[i].taskData);
                outTask[i].taskData = 0x0;
                break;
            }
            default: {
                outTask[i].taskOperation = INPOUT_IRQ_OCCURRED;
                outTask[i].taskPort = 0xAA;
                outTask[i].taskData = DevContext->InterruptCount;
                break;
            }
        }
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC,
            "%!FUNC! <---- parsed task: code==0x%04x, port==0x%04x, data==0x%08x\n",
            outTask[i].taskOperation, outTask[i].taskPort, outTask[i].taskData);

    }
    //
    // Complete the IOCTL_OSR_INVERT_NOTIFICATION with success, indicating
    // we're returning a longword of data in the user's OutBuffer
    // 
    status = STATUS_SUCCESS;
    opInfo = (ULONG)inBufferSize;

    //
    // And now... NOTIFY the user about the event.  We do this just
    // by completing the dequeued Request.
    // 
queue_completion:
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC,
                "%!FUNC! <--- Notifying User: (P=0x%p, S=%d bytes), Status=0x%x, opSize=%d\n",
                outBuf, (int)outBufferSize, status, (UINT32)opInfo);
    WdfRequestCompleteWithInformation(notifyRequest, status, opInfo);

}