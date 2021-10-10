/*++

Copyright (c) Gvozdev A. Feodor.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    IsrDpc.c

Abstract:

    Contains routines related to interrupt and dpc handling.

Environment:

    Kernel mode

--*/

#include "driver.h"
#include "trace.h"
#include "irqhandler.tmh"

NTSTATUS
inpOutNgInterruptCreate(
    IN PINPOUTNG_CONTEXT DevExt
)
/*++
Routine Description:

    Configure and create the WDFINTERRUPT object.
    This routine is called by EvtDeviceAdd callback.

Arguments:

    DevExt      Pointer to our DEVICE_EXTENSION

Return Value:

    NTSTATUS code

--*/
{
    NTSTATUS                    status;
    WDF_INTERRUPT_CONFIG        IsrConfig;
    WDF_OBJECT_ATTRIBUTES       IsrLockAttr;

    WDF_INTERRUPT_CONFIG_INIT(&IsrConfig,
        inpOutNgEvtInterruptIsr,
        inpOutNgEvtInterruptDpc);

    WDF_OBJECT_ATTRIBUTES_INIT(&IsrLockAttr);
    IsrLockAttr.ParentObject = DevExt->Interrupt;
    status = WdfSpinLockCreate(&IsrLockAttr, &DevExt->IsrLock);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_IRQ,
            "WdfIsrLockCreate failed: %!STATUS!", status);
    }

    IsrConfig.EvtInterruptEnable = inpOutNgEvtInterruptEnable;
    IsrConfig.EvtInterruptDisable = inpOutNgEvtInterruptDisable;

    IsrConfig.AutomaticSerialization = TRUE;
    IsrConfig.ShareVector = TRUE;
    
    IsrConfig.SpinLock = DevExt->IsrLock;

    //
    // Unlike WDM, framework driver should create interrupt object in EvtDeviceAdd and
    // let the framework do the resource parsing and registration of ISR with the kernel.
    // Framework connects the interrupt after invoking the EvtDeviceD0Entry callback
    // and disconnect before invoking EvtDeviceD0Exit. EvtInterruptEnable is called after
    // the interrupt interrupt is connected and EvtInterruptDisable before the interrupt is
    // disconnected.
    //
    status = WdfInterruptCreate(DevExt->Device,
        &IsrConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &DevExt->Interrupt);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_IRQ,
            "WdfInterruptCreate failed: %!STATUS!", status);
    }

    return status;
}

BOOLEAN
inpOutNgEvtInterruptIsr(
    IN WDFINTERRUPT Interrupt,
    IN ULONG        MessageID
)
/*++
Routine Description:

    Interrupt handler for this driver. Called at DIRQL level when the
    device or another device sharing the same interrupt line asserts
    the interrupt. The driver first checks the device to make sure whether
    this interrupt is generated by its device and if so clear the interrupt
    register to disable further generation of interrupts and queue a
    DPC to do other I/O work related to interrupt - such as reading
    the device memory, starting a DMA transaction, coping it to
    the request buffer and completing the request, etc.

Arguments:

    Interupt   - Handle to WDFINTERRUPT Object for this device.
    MessageID  - MSI message ID (always 0 in this configuration)

Return Value:

     TRUE   --  This device generated the interrupt.
     FALSE  --  This device did not generated this interrupt.

--*/
{
    PINPOUTNG_CONTEXT   devExt    = NULL;
    BOOLEAN             irqQueued = FALSE;

    UNREFERENCED_PARAMETER(MessageID);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_IRQ, "--> InpOutNgInterruptHandler");

    devExt = inpOutNgGetContext(WdfInterruptGetDevice(Interrupt));

    if (devExt) {
        __outbyte((USHORT)0x333, (UCHAR)0xff);   // ����� ����������
        irqQueued = WdfInterruptQueueDpcForIsr(devExt->Interrupt);
    }
    else {
        TraceEvents(TRACE_LEVEL_WARNING, TRACE_IRQ, "%!FUNC! No Device extension!");
    }


    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_IRQ, "<-- InpOutNgInterruptHandler, %s", irqQueued ? "Queued" : "NOT queued");

    return irqQueued;
}

VOID
inpOutNgEvtInterruptDpc(
    WDFINTERRUPT Interrupt,
    WDFOBJECT    Device
)
/*++

Routine Description:

    DPC callback for ISR. Please note that on a multiprocessor system,
    you could have more than one DPCs running simulataneously on
    multiple processors. So if you are accesing any global resources
    make sure to synchrnonize the accesses with a spinlock.

Arguments:

    Interupt  - Handle to WDFINTERRUPT Object for this device.
    Device    - WDFDEVICE object passed to InterruptCreate

Return Value:

--*/
{
    UNREFERENCED_PARAMETER(Device);

    PINPOUTNG_CONTEXT        devContext;

    //
    // Acquire this device's InterruptSpinLock.
    //
    //WdfInterruptAcquireLock(Interrupt);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_IRQ_DPC, "--> %!FUNC!");

    devContext = inpOutNgGetContext(WdfInterruptGetDevice(Interrupt));
    
    if (devContext) {

        inpOutNgNotify(devContext);

        devContext->InterruptCount++;
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_IRQ_DPC, "IRQ Count %d", devContext->InterruptCount);
    }
    else {
        TraceEvents(TRACE_LEVEL_WARNING, TRACE_IRQ_DPC, "%!FUNC! No Device extension!");
    }
    //WdfInterruptReleaseLock(Interrupt);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_IRQ_DPC, "<-- %!FUNC!");

    return;
}

NTSTATUS
inpOutNgEvtInterruptEnable(
    IN WDFINTERRUPT Interrupt,
    IN WDFDEVICE    Device
)
/*++

Routine Description:

    Called by the framework at DIRQL immediately after registering the ISR with the kernel
    by calling IoConnectInterrupt.

Return Value:

    NTSTATUS
--*/
{
    PINPOUTNG_CONTEXT  devExt;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_IRQ,
        "%!FUNC!: Interrupt 0x%p, Device 0x%p\n",
        Interrupt, Device);

    devExt = inpOutNgGetContext(WdfInterruptGetDevice(Interrupt));

/*
    intCSR.ulong = READ_REGISTER_ULONG((PULONG)&devExt->Regs->Int_Csr);

    intCSR.bits.PciIntEnable = TRUE;

    WRITE_REGISTER_ULONG((PULONG)&devExt->Regs->Int_Csr,
        intCSR.ulong);
*/
    return STATUS_SUCCESS;
}

NTSTATUS
inpOutNgEvtInterruptDisable(
    IN WDFINTERRUPT Interrupt,
    IN WDFDEVICE    Device
)
/*++

Routine Description:

    Called by the framework at DIRQL before Deregistering the ISR with the kernel
    by calling IoDisconnectInterrupt.

Return Value:

    NTSTATUS
--*/
{
    PINPOUTNG_CONTEXT  devExt;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_IRQ,
        "%!FUNC!: Interrupt 0x%p, Device 0x%p\n",
        Interrupt, Device);

    devExt = inpOutNgGetContext(WdfInterruptGetDevice(Interrupt));

    return STATUS_SUCCESS;
}
