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
#pragma alloc_text (PAGE, inpOutNgEvtIoDeviceControl)
#pragma alloc_text (PAGE, inpOutNgNotify)
#endif
/**
 * @brief ������������� �������� ��������� ��������
 * @param Device ��������� ����������, �������������� ���������
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
    // �������� ������� ������� ��� ����������, � ������� �������������� �������,
    // �� ������������������ �� ��������������� ����� WdfDeviceConfigureRequestDispatching
    // � ���������� ����� ������ �������.
    //
    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(
        &queueConfig,
        WdfIoQueueDispatchParallel
        );

    queueConfig.EvtIoDeviceControl = inpOutNgEvtIoDeviceControl;
    queueConfig.EvtIoStop = inpOutNgEvtIoStop;

    status = WdfIoQueueCreate(
        Device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->cntrlQueue
        );

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfDefaultQueueCreate failed %!STATUS!", status);
        return status;
    }

    //
    // �������� ������� ��� ��������� �������� �� ����������� �����������.
    // ����� ����������� ����������� ����� ��������� ������� ��������� ������
    // (Inverted call model) ������� � ������ OSR �� ������
    // https://www.osr.com/nt-insider/2013-issue1/inverted-call-model-kmdf/
    // 
    // � ������ ������� ����� ���������� �������, ������������� �� ������������� �������
    // (���������� ��� ��������������)
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
        WdfIoQueueDispatchManual);

    queueConfig.EvtIoStop = inpOutNgEvtIoStop;

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
 * @brief ������� ��������� �������� �������� �� �������� �����/������
 * @param Queue � ���������� ����������� ������� ����������� ��������
 * @param Request � ���������� �������
 * @param OutputBufferLength � ������ ��������� ������ � ������
 * @param InputBufferLength � ������ �������� ������ � ������
 * @param IoControlCode � ��� �������� �����/������
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
                "%!FUNC! Queue 0x%p, Request 0x%p OutputBufferLength %d InputBufferLength %d IoControlCode %08x", 
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
    PAGED_CODE();

    // ��������� ������ �� �������� ���������� ����� ������������� �������...
    devContext = inpOutNgGetContext(WdfIoQueueGetDevice(Queue));

    // ���� ������ �������� ������ ���� � ��� �� ������ ������, ����� ������ ������
    if (!InputBufferLength && (IoControlCode != IOCTL_GET_DRVINFO))
    {
        WdfRequestComplete(Request, STATUS_INVALID_PARAMETER);
        return;
    }
    
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

    // ���������, ��� ������� ��������� ��������� ������ �������� ������
    ASSERT(inBuffersize >= InputBufferLength);
    
    // � ��������� �� ���� �� �������
    ASSERT(inBuf != NULL);

    // ��������� �� ������� ����� �������������� ��������� ��� ������������ ������� */
    inData = (p_inPortData_t)inBuf;

    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Data from User : (0x%p) 0x%08x\n", inBuf, *inBuf);

    // 
    // ���� ����� ��������� ������ �� ����, ��������� �������� ��������� �� ���� � ���������,
    // ��� ������� ���������� ������ ��������� � ��������� �� �������
    //
    if (OutputBufferLength != 0)
    {
        status = WdfRequestRetrieveOutputBuffer(Request, 0, &outBuf, &outBuffersize);
        if (!NT_SUCCESS(status)) {
            status = STATUS_INSUFFICIENT_RESOURCES;
        }
        ASSERT(outBuffersize >= OutputBufferLength);
        ASSERT(outBuf != NULL);
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Data to User : (0x%p)\n", outBuf);
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
            // ������������ �������� ������ ���� �������� �������,
            // ���������� ���������, ��� �������� ����� ������ ������� ��������
            // ����� ���������������� ������� � ������� ��������.
            // 
            if (!(outBuffersize>=sizeof(port_task_t))) {

                opInfo = 0;
                status = STATUS_BUFFER_TOO_SMALL;
                break;
            }

            status = WdfRequestForwardToIoQueue(Request,
                devContext->asyncQueue);

            //
            // ���� ������ �� ����� ���� ������������� � ������� ��������,
            // ��������� ��� ���������. ������ �������� ����� ��������������� ���� ������
            // ��������������� � ������� (WdfRequestForwardToIoQueue).
            // 
            if (!NT_SUCCESS(status)) {
                opInfo = 0;
                break;
            }

            //
            // *** ������ ��������� � �������� ***
            //     ������ �������� ����� ��������� �� ��� ����������,
            //     ����� �� ������� �����������.
            //
            return;
        }

        case IOCTL_TEST_ASYNC: {
            inpOutNgNotify(devContext);
            return;
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

    WdfRequestCompleteWithInformation(Request,
        status,
        opInfo);

    return;
}

/**
 * @brief This event is invoked for a power-managed queue before the device leaves the working state (D0).
 * @param Queue �  ���������� ����������� ������� ����������� ��������
 * @param Request � ���������� �������
 * @param ActionFlags � A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
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
 * @brief ������� ����������� ������� ����������� �����������
 * @param DevContext � ���������� ������ ����������
*/
VOID
inpOutNgNotify(PINPOUTNG_CONTEXT DevContext)
{
    NTSTATUS      status;
    ULONG_PTR     opInfo;
    WDFREQUEST    notifyRequest;

    PULONG        inBuf = NULL;
    size_t        inBufferSize;
    p_port_task_t inTask;

    PULONG        outBuf = NULL;
    size_t        outBufferSize;
    p_port_task_t outTask;

    size_t        taskSize;
    size_t        i;

    status = WdfIoQueueRetrieveNextRequest(
        DevContext->asyncQueue,
        &notifyRequest);

    //
    // �������� �� ������� ��������� �������� � �������
    // 
    if (!NT_SUCCESS(status)) {

        //
        // �������� ���. ������� ������� ������ �� ������� �������� �� �����.
        // ��������, � ������� �������� ��� ��������. � ����� ������ � �� �����
        // 
        return;
    }

    //
    // ��������� ����������� �� ������� �������.
    // 

    //
    // ��������� ���������� �� ������ ����� / �������� ���������� �� ����������.
    // ���������� � ������ �������.
    // 

    status = WdfRequestRetrieveInputBuffer(notifyRequest, 0, &inBuf, &inBufferSize);
    //
    // �������� �� ������� �������� ������
    // 
    if (!NT_SUCCESS(status)) {

        //
        // ����� � �������� ����, ��������� ������ �� �������� � ��� ��������.
        // 
        status = STATUS_INSUFFICIENT_RESOURCES;
        opInfo = 0;
        goto queue_completion;
    }
    else {
        // ���������, ��� ������ �������� ������ ������ 32-� ��������
        ASSERT(inBufferSize >= sizeof(port_task_t));

        // ���������, ��� ��������� �� ����� �� �������
        ASSERT(inBuf != NULL);
        
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! Data from User : 0x%p, %d bytes\n", inBuf, (int)inBufferSize);
        //
        // We successfully retrieved a Request from the notification Queue
        // AND we retrieved an output buffer into which to return some
        // additional information.
        // 
        status = WdfRequestRetrieveOutputBuffer(notifyRequest, 0, &outBuf, &outBufferSize);
        if (!NT_SUCCESS(status)) {
            opInfo = 0;
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto queue_completion;
        }

        ASSERT(outBufferSize >= sizeof(port_task_t));
        ASSERT(outBuf != NULL);
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_ASYNC, "%!FUNC! Data to User : 0x%p, %d bytes\n", outBuf, (int)outBufferSize);
        
        if (outBufferSize < inBufferSize)
        {
            opInfo = 0;
            status = STATUS_INSUFFICIENT_RESOURCES;
            goto queue_completion;
        }

        inTask = (p_port_task_t)inBuf;
        outTask = (p_port_task_t)outBuf;

        taskSize = inBufferSize / sizeof(port_task_t);

        for (i = 0; i < taskSize; i++)
        {
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
                    outTask[i].taskOperation = INPOUT_READ8_ACK;
                    outTask[i].taskData = __inword(inTask[i].taskPort);
                    break;
                }
                case INPOUT_WRITE16: {
                    outTask[i].taskOperation = INPOUT_WRITE8_ACK;
                    __outword(inTask[i].taskPort, (USHORT)(inTask[i].taskData & 0x0000ffff));
                    outTask[i].taskData = 0x0;
                    break;
                }
                case INPOUT_READ32: {
                    outTask[i].taskOperation = INPOUT_READ8_ACK;
                    outTask[i].taskData = __indword(inTask[i].taskPort);
                    break;
                }
                case INPOUT_WRITE32: {
                    outTask[i].taskOperation = INPOUT_WRITE8_ACK;
                    __outdword(inTask[i].taskPort, inTask[i].taskData);
                    outTask[i].taskData = 0x0;
                    break;
                }
                default: {
                    outTask[i].taskOperation = INPOUT_ACK;
                    outTask[i].taskPort = 0xAA;
                    outTask[i].taskData = 0xA5;
                    break;
                }
            }
        }
        //
        // Complete the IOCTL_OSR_INVERT_NOTIFICATION with success, indicating
        // we're returning a longword of data in the user's OutBuffer
        // 
        status = STATUS_SUCCESS;
        opInfo = inBufferSize;
    }

    //
    // And now... NOTIFY the user about the event.  We do this just
    // by completing the dequeued Request.
    // 
queue_completion:
    WdfRequestCompleteWithInformation(notifyRequest, status, opInfo);

}