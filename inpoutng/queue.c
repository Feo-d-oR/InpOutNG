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

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, inpOutNgQueueInitialize)
#pragma alloc_text (PAGE, inpOutNgEvtIoDeviceControl)
#endif

#pragma pack(push)
#pragma pack(1)
typedef struct S_InPortData
{
    USHORT addr;
    union U_inPortVal
    {
        UCHAR  inChar;
        USHORT inShrt;
        ULONG  inLong;
    } val;
} inPortData_t, *p_inPortData_t;

typedef struct S_OutPortData
{
    union U_outPortVal
    {
        UCHAR  outChar;
        USHORT outShrt;
        ULONG  outLong;
    } val;
} outPortData_t, * p_outPortData_t;
#pragma pack()

NTSTATUS
inpOutNgQueueInitialize(
    _In_ WDFDEVICE Device
    )
/*++

Routine Description:

     The I/O dispatch callbacks for the frameworks device object
     are configured in this function.

     A single default I/O Queue is configured for parallel request
     processing, and a driver context memory allocation is created
     to hold our structure QUEUE_CONTEXT.

Arguments:

    Device - Handle to a framework device object.

Return Value:

    VOID

--*/
{
    WDFQUEUE queue;
    NTSTATUS status;
    WDF_IO_QUEUE_CONFIG queueConfig;

    PAGED_CODE();

    //
    // Configure a default queue so that requests that are not
    // configure-fowarded using WdfDeviceConfigureRequestDispatching to goto
    // other queues get dispatched here.
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
                 &queue
                 );

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_QUEUE, "WdfIoQueueCreate failed %!STATUS!", status);
        return status;
    }

    return status;
}

VOID
inpOutNgEvtIoDeviceControl(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ size_t OutputBufferLength,
    _In_ size_t InputBufferLength,
    _In_ ULONG IoControlCode
    )
/*++

Routine Description:

    This event is invoked when the framework receives IRP_MJ_DEVICE_CONTROL request.

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    OutputBufferLength - Size of the output buffer in bytes

    InputBufferLength - Size of the input buffer in bytes

    IoControlCode - I/O control code.

Return Value:

    VOID

--*/
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

    PAGED_CODE();

    /* Если размер входного буфера ноль, здесь делать нечего */
    if (!InputBufferLength)
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

    /* Проверить, что система правильно посчитала размер входного буфера */
    ASSERT(inBuffersize >= InputBufferLength);
    
    /* И указатель на него не нулевой */
    ASSERT(inBuf != NULL);

    /* «Сослать» на входной буфер типизированный указатель для последующего разбора */
    inData = (p_inPortData_t)inBuf;

    //
    // Read the input buffer content.
    // We are using the following function to print characters instead
    // TraceEvents with %s format because the string we get may or
    // may not be null terminated. The buffer may contain non-printable
    // characters also.
    //
    TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Data from User : (0x%p) 0x%08x\n", inBuf, *inBuf);

    /* Если длина выходного буфера не ноль, требуется получиьт указатель на него и проверить,
        что система определила размер корректно и указатель не нулевой */
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
        case IOCTL_READ_PORT_UCHAR:
            if (!((inBuffersize >= 2) && (outBuffersize >= 1)))
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

        case IOCTL_WRITE_PORT_UCHAR:
            if (!(inBuffersize >= 3))
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

        case IOCTL_READ_PORT_USHORT:
            if (!((inBuffersize >= 2) && (outBuffersize >= 2)))
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

        case IOCTL_WRITE_PORT_USHORT:
            if (!(inBuffersize >= 4))
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

        case IOCTL_READ_PORT_ULONG:
            if (!((inBuffersize >= 4) && (outBuffersize >= 4)))
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

        case IOCTL_WRITE_PORT_ULONG:
            if (!(inBuffersize >= 8))
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
        default:
            opInfo = 0;
            status = STATUS_UNSUCCESSFUL;
            TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Unknown IOCTL 0x%08x, Status=0x%08x, Return=%d\n", IoControlCode, status, (UINT32)opInfo);
            break;

    }
    
    if ((opInfo > 0) && (outBuffersize > 0))
    {
        TraceEvents(TRACE_LEVEL_VERBOSE, TRACE_IOCTL, "%!FUNC! Committing data to User: (P=0x%p, S=%d bytes), Status=0x%x, opSize=%d\n",
            outBuf, (int)outBuffersize, status, (UINT32)opInfo);
        RtlCopyMemory(outBuf, &outData, opInfo);
    }

    WdfRequestSetInformation(Request, opInfo);
    WdfRequestComplete(Request, status);

    return;
}

VOID
inpOutNgEvtIoStop(
    _In_ WDFQUEUE Queue,
    _In_ WDFREQUEST Request,
    _In_ ULONG ActionFlags
)
/*++

Routine Description:

    This event is invoked for a power-managed queue before the device leaves the working state (D0).

Arguments:

    Queue -  Handle to the framework queue object that is associated with the
             I/O request.

    Request - Handle to a framework request object.

    ActionFlags - A bitwise OR of one or more WDF_REQUEST_STOP_ACTION_FLAGS-typed flags
                  that identify the reason that the callback function is being called
                  and whether the request is cancelable.

Return Value:

    VOID

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, 
                TRACE_QUEUE, 
                "%!FUNC! Queue 0x%p, Request 0x%p ActionFlags %d", 
                Queue, Request, ActionFlags);

    //
    // In most cases, the EvtIoStop callback function completes, cancels, or postpones
    // further processing of the I/O request.
    //
    // Typically, the driver uses the following rules:
    //
    // - If the driver owns the I/O request, it calls WdfRequestUnmarkCancelable
    //   (if the request is cancelable) and either calls WdfRequestStopAcknowledge
    //   with a Requeue value of TRUE, or it calls WdfRequestComplete with a
    //   completion status value of STATUS_SUCCESS or STATUS_CANCELLED.
    //
    //   Before it can call these methods safely, the driver must make sure that
    //   its implementation of EvtIoStop has exclusive access to the request.
    //
    //   In order to do that, the driver must synchronize access to the request
    //   to prevent other threads from manipulating the request concurrently.
    //   The synchronization method you choose will depend on your driver's design.
    //
    //   For example, if the request is held in a shared context, the EvtIoStop callback
    //   might acquire an internal driver lock, take the request from the shared context,
    //   and then release the lock. At this point, the EvtIoStop callback owns the request
    //   and can safely complete or requeue the request.
    //
    // - If the driver has forwarded the I/O request to an I/O target, it either calls
    //   WdfRequestCancelSentRequest to attempt to cancel the request, or it postpones
    //   further processing of the request and calls WdfRequestStopAcknowledge with
    //   a Requeue value of FALSE.
    //
    // A driver might choose to take no action in EvtIoStop for requests that are
    // guaranteed to complete in a small amount of time.
    //
    // In this case, the framework waits until the specified request is complete
    // before moving the device (or system) to a lower power state or removing the device.
    // Potentially, this inaction can prevent a system from entering its hibernation state
    // or another low system power state. In extreme cases, it can cause the system
    // to crash with bugcheck code 9F.
    //

    return;
}

NTSTATUS MapPhysicalMemoryToLinearSpace(PVOID pPhysAddress,
    SIZE_T PhysMemSizeInBytes,
    PVOID* ppPhysMemLin,
    HANDLE* pPhysicalMemoryHandle)
{
    UNICODE_STRING     PhysicalMemoryUnicodeString;
    PVOID              PhysicalMemorySection = NULL;
    OBJECT_ATTRIBUTES  ObjectAttributes;
    PHYSICAL_ADDRESS   ViewBase;
    NTSTATUS           ntStatus;
    LARGE_INTEGER      pStartPhysAddress;
    LARGE_INTEGER      pEndPhysAddress;
    LARGE_INTEGER      MappingLength;
    BOOLEAN            Result1, Result2;
    ULONG              IsIOSpace;
    unsigned char* pbPhysMemLin = NULL;

    RtlInitUnicodeString(&PhysicalMemoryUnicodeString, L"\\Device\\PhysicalMemory");
    InitializeObjectAttributes(&ObjectAttributes,
        &PhysicalMemoryUnicodeString,
        OBJ_CASE_INSENSITIVE,
        (HANDLE)NULL,
        (PSECURITY_DESCRIPTOR)NULL);

    *pPhysicalMemoryHandle = NULL;
    ntStatus = ZwOpenSection(pPhysicalMemoryHandle,
        SECTION_ALL_ACCESS,
        &ObjectAttributes);

    if (NT_SUCCESS(ntStatus))
    {
        ntStatus = ObReferenceObjectByHandle(*pPhysicalMemoryHandle,
            SECTION_ALL_ACCESS,
            (POBJECT_TYPE)NULL,
            KernelMode,
            &PhysicalMemorySection,
            (POBJECT_HANDLE_INFORMATION)NULL);

        if (NT_SUCCESS(ntStatus))
        {
            pStartPhysAddress.QuadPart = (ULONGLONG)pPhysAddress;
            pEndPhysAddress.QuadPart = pStartPhysAddress.QuadPart + (LONGLONG)PhysMemSizeInBytes;
            IsIOSpace = 0;
            Result1 = 1;//!!HalTranslateBusAddress(1, 0, pStartPhysAddress, &IsIOSpace, &pStartPhysAddress);
            IsIOSpace = 0;
            Result2 = 2;//!!HalTranslateBusAddress(1, 0, pEndPhysAddress, &IsIOSpace, &pEndPhysAddress);

            if (Result1 && Result2)
            {

                MappingLength.QuadPart = pEndPhysAddress.QuadPart - pStartPhysAddress.QuadPart;

                if (MappingLength.LowPart)
                {
                    // Let ZwMapViewOfSection pick a linear address
#ifdef _AMD64_
                    PhysMemSizeInBytes = MappingLength.QuadPart;
#else
                    PhysMemSizeInBytes = MappingLength.LowPart;
#endif
                    ViewBase = pStartPhysAddress;
                    ntStatus = ZwMapViewOfSection(*pPhysicalMemoryHandle,
                        (HANDLE)-1,
                        &pbPhysMemLin,
                        0L,
                        PhysMemSizeInBytes,
                        &ViewBase,
                        &PhysMemSizeInBytes,
                        ViewShare,
                        0,
                        PAGE_READWRITE | PAGE_NOCACHE);

                    if (NT_SUCCESS(ntStatus))
                    {
                        pbPhysMemLin += (ULONG)pStartPhysAddress.LowPart - (ULONG)ViewBase.LowPart;
                        *ppPhysMemLin = pbPhysMemLin;
                    }
                }
            }
        }
    }

    if (!NT_SUCCESS(ntStatus))
        ZwClose(*pPhysicalMemoryHandle);

    return ntStatus;
}


NTSTATUS UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, PVOID pPhysMemLin)
{
    NTSTATUS ntStatus;
    ntStatus = ZwUnmapViewOfSection((HANDLE)-1, pPhysMemLin);
    ZwClose(PhysicalMemoryHandle);
    return ntStatus;
}
