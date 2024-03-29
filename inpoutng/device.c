/*++

Module Name:

    device.c - Device handling events for example driver.

Abstract:

   This file contains the device entry points and callbacks.
    
Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "device.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, inpOutNgCreateDevice)
#pragma alloc_text (PAGE, inpOutNgEvtDevicePrepareHardware)
#pragma alloc_text (PAGE, inpOutNgEvtDeviceReleaseHardware)
#pragma alloc_text (PAGE, inpOutNgEvtDeviceD0Exit)
#pragma alloc_text (PAGE, inpOutNgSetIdleAndWakeSettings)
#pragma alloc_text (PAGE, inpOutNgPrepareHardware)
#pragma alloc_text (PAGE, inpOutNgInitializeDeviceContext)
#endif

NTSTATUS
inpOutNgCreateDevice(
    _In_ PWDFDEVICE_INIT DeviceInit,
    _Out_ WDFDEVICE* Device
    )
/*++

Routine Description:

    Worker routine called to create a device and its software resources.

Arguments:

    DeviceInit - Pointer to an opaque init structure. Memory for this
                    structure will be freed by the framework when the WdfDeviceCreate
                    succeeds. So don't access the structure after that point.

Return Value:

    NTSTATUS

--*/
{
    WDF_OBJECT_ATTRIBUTES devAttrib;
    PINPOUTNG_CONTEXT     devContext;
    PNP_BUS_INFORMATION   busInfo;
    WDFDEVICE             device;
    NTSTATUS              status;

//!!    PDEVICE_OBJECT deviceObject;
//!!    NTSTATUS status;

    WCHAR NameBuffer[] = L"\\Device\\inpoutng";

    UNICODE_STRING uniNameString;

    PAGED_CODE();

    //
    // Initialize WPP Tracing
    //
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_BUS_EXTENDER);
    //WdfDeviceInitSetExclusive(DeviceInit, TRUE);
    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoBuffered);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&devAttrib, INPOUTNG_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &devAttrib, &device);

    if (NT_SUCCESS(status)) {

        *Device = device;
        RtlInitUnicodeString(&uniNameString, NameBuffer);

        //
        // Get a pointer to the device context structure that we just associated
        // with the device object. We define this structure in the device.h
        // header file. DeviceGetContext is an inline function generated by
        // using the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in device.h.
        // This function will do the type checking and return the device context.
        // If you pass a wrong object handle it will return NULL and assert if
        // run under framework verifier mode.
        //
        devContext = inpOutNgGetContext(device);

        //
        // Initialize the context.
        //
        devContext->Device = device;
        devContext->HwErrCount = 0;

        //
        // Create a device interface so that applications can find and talk
        // to us.
        //
        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_INPOUTNG,
            NULL // ReferenceString
            );
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE,
                "<-- DeviceCreateDeviceInterface "
                "failed %!STATUS!", status);
            return status;
        }
        else {
            //
            // Initialize the I/O Package and any Queues
            //
            status = inpOutNgQueueInitialize(device);
        }

        //
        // This value is used in responding to the IRP_MN_QUERY_BUS_INFORMATION
        // for the child devices. This is an optional information provided to
        // uniquely idenitfy the bus the device is connected.
        //
        busInfo.BusTypeGuid = GUID_DEVCLASS_SYSTEM;
        busInfo.LegacyBusType = Isa;
        busInfo.BusNumber = 0;

        WdfDeviceSetBusInformationForChildren(device, &busInfo);
        
        //
        //Create ISR�
        //
        status = inpOutNgInterruptCreate(devContext);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDeviceCreate ISR creation failed %!STATUS!", status);
            return status;
        }

        status = WdfDeviceCreateSymbolicLink(device, &uniNameString);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDeviceCreate NT name creation failed %!STATUS!", status);
        }
    }

    return status;
}

NTSTATUS
inpOutNgInitializeDeviceContext(
    _In_ PINPOUTNG_CONTEXT devContext
)
/*++
Routine Description:

    This routine is called by EvtDeviceAdd. Here the device context is
    initialized and all the software resources required by the device is
    allocated.

Arguments:

    devContext     Pointer to the Device Extension

Return Value:

     NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(devContext);

    NTSTATUS    status = STATUS_SUCCESS;

    //WDF_IO_QUEUE_CONFIG  queueConfig;

    PAGED_CODE();
    //Need to realize truth about queues. Maybe this is not needed for a simple ISA transfers.

#if 0
    //
    // Setup a queue to handle only IRP_MJ_WRITE requests in Sequential
    // dispatch mode. This mode ensures there is only one write request
    // outstanding in the driver at any time. Framework will present the next
    // request only if the current request is completed.
    // Since we have configured the queue to dispatch all the specific requests
    // we care about, we don't need a default queue.  A default queue is
    // used to receive requests that are not preconfigured to goto
    // a specific queue.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
        WdfIoQueueDispatchSequential);

    queueConfig.EvtIoWrite = inpOutNgEvtIoWrite;

    //
    // Static Driver Verifier (SDV) displays a warning if it doesn't find the 
    // EvtIoStop callback on a power-managed queue. The 'assume' below lets 
    // SDV know not to worry about the EvtIoStop.
    // If not explicitly set, the framework creates power-managed queues when 
    // the device is not a filter driver.  Normally the EvtIoStop is required
    // for power-managed queues, but for this driver it is not need b/c the 
    // driver doesn't hold on to the requests for long time or forward them to
    // other drivers. 
    // If the EvtIoStop callback is not implemented, the framework 
    // waits for all in-flight (driver owned) requests to be done before 
    // moving the device in the Dx/sleep states or before removing the device,
    // which is the correct behavior for this type of driver.
    // If the requests were taking an undetermined amount of time to complete,
    // or the requests were forwarded to a lower driver/another stack, the 
    // queue should have an EvtIoStop/EvtIoResume.
    //
    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(devContext->Device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->WriteQueue);
    __analysis_assume(queueConfig.EvtIoStop == 0);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate failed: %!STATUS!", status);
        return status;
    }

    //
    // Set the Write Queue forwarding for IRP_MJ_WRITE requests.
    //
    status = WdfDeviceConfigureRequestDispatching(devContext->Device,
        devContext->WriteQueue,
        WdfRequestTypeWrite);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "DeviceConfigureRequestDispatching failed: %!STATUS!", status);
        return status;
    }


    //
    // Create a new IO Queue for IRP_MJ_READ requests in sequential mode.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
        WdfIoQueueDispatchSequential);

    queueConfig.EvtIoRead = inpOutNgEvtIoRead;

    //
    // By default, Static Driver Verifier (SDV) displays a warning if it 
    // doesn't find the EvtIoStop callback on a power-managed queue. 
    // The 'assume' below causes SDV to suppress this warning. If the driver 
    // has not explicitly set PowerManaged to WdfFalse, the framework creates
    // power-managed queues when the device is not a filter driver.  Normally 
    // the EvtIoStop is required for power-managed queues, but for this driver
    // it is not needed b/c the driver doesn't hold on to the requests for 
    // long time or forward them to other drivers. 
    // If the EvtIoStop callback is not implemented, the framework waits for
    // all driver-owned requests to be done before moving in the Dx/sleep 
    // states or before removing the device, which is the correct behavior 
    // for this type of driver. If the requests were taking an indeterminate
    // amount of time to complete, or if the driver forwarded the requests
    // to a lower driver/another stack, the queue should have an 
    // EvtIoStop/EvtIoResume.
    //
    __analysis_assume(queueConfig.EvtIoStop != 0);
    status = WdfIoQueueCreate(devContext->Device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->ReadQueue);
    __analysis_assume(queueConfig.EvtIoStop == 0);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate failed: %!STATUS!", status);
        return status;
    }

    //
    // Set the Read Queue forwarding for IRP_MJ_READ requests.
    //
    status = WdfDeviceConfigureRequestDispatching(devContext->Device,
        devContext->ReadQueue,
        WdfRequestTypeRead);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "DeviceConfigureRequestDispatching failed: %!STATUS!", status);
        return status;
    }


    //
    // Create a new IO Queue for IOCTLs in sequential mode.
    //
    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig,
        WdfIoQueueDispatchSequential);

    queueConfig.EvtIoDeviceControl = InpOutNgEvtIoDeviceControl;

    status = WdfIoQueueCreate(devContext->Device,
        &queueConfig,
        WDF_NO_OBJECT_ATTRIBUTES,
        &devContext->ControlQueue);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate failed: %!STATUS!",
            status);
        return status;
    }

    //
    // Set the Control Queue forwarding for IOCTL requests.
    //
    status = WdfDeviceConfigureRequestDispatching(devContext->Device,
        devContext->ControlQueue,
        WdfRequestTypeDeviceControl);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceConfigureRequestDispatching failed: %!STATUS!",
            status);
        return status;
    }


    //
    // Create a WDFINTERRUPT object.
    //
    status = inpOutNgInterruptCreate(devContext);

    if (!NT_SUCCESS(status)) {
        return status;
    }

#endif

    return status;
}


VOID
inpOutNgCleanupDeviceContext(
    _In_ PINPOUTNG_CONTEXT devContext
)
/*++

Routine Description:

    Frees allocated memory that was saved in the
    WDFDEVICE's context, before the device object
    is deleted.

Arguments:

    devContext - Pointer to our DEVICE_EXTENSION

Return Value:

     None

--*/
{
    UNREFERENCED_PARAMETER(devContext);
}

NTSTATUS
inpOutNgEvtDevicePrepareHardware(
    WDFDEVICE       Device,
    WDFCMRESLIST   Resources,
    WDFCMRESLIST   ResourcesTranslated
)
/*++

Routine Description:

    Performs whatever initialization is needed to setup the device, setting up
    a DMA channel or mapping any I/O port resources.  This will only be called
    as a device starts or restarts, not every time the device moves into the D0
    state.  Consequently, most hardware initialization belongs elsewhere.

Arguments:

    Device - A handle to the WDFDEVICE

    Resources - The raw PnP resources associated with the device.  Most of the
        time, these aren't useful for a PCI device.

    ResourcesTranslated - The translated PnP resources associated with the
        device.  This is what is important to a PCI device.

Return Value:

    NT status code - failure will result in the device stack being torn down

--*/
{
    NTSTATUS          status = STATUS_SUCCESS;
    PINPOUTNG_CONTEXT   devContext;

    UNREFERENCED_PARAMETER(Resources);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "--> %!FUNC!");

    devContext = inpOutNgGetContext(Device);

    status = inpOutNgPrepareHardware(devContext, ResourcesTranslated);
    if (!NT_SUCCESS(status)) {
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "<-- %!FUNC!, status %!STATUS!", status);

    return status;
}

NTSTATUS
inpOutNgEvtDeviceReleaseHardware(
    _In_  WDFDEVICE Device,
    _In_  WDFCMRESLIST ResourcesTranslated
)
/*++

Routine Description:

    Unmap the resources that were mapped in inpOutNgEvtDevicePrepareHardware.
    This will only be called when the device stopped for resource rebalance,
    surprise-removed or query-removed.

Arguments:

    Device - A handle to the WDFDEVICE

    ResourcesTranslated - The translated PnP resources associated with the
        device.  This is what is important to a PCI device.

Return Value:

    NT status code - failure will result in the device stack being torn down

--*/
{
    PINPOUTNG_CONTEXT   devContext;
    NTSTATUS status = STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(ResourcesTranslated);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "--> %!FUNC!");

    devContext = inpOutNgGetContext(Device);

    if (devContext) {
        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "Context found but nothing to do with IO spaces yet...");
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "<-- %!FUNC!");

    return status;
}

NTSTATUS
inpOutNgEvtDeviceD0Entry(
    _In_  WDFDEVICE Device,
    _In_  WDF_POWER_DEVICE_STATE PreviousState
)
/*++

Routine Description:

    This routine prepares the device for use.  It is called whenever the device
    enters the D0 state, which happens when the device is started, when it is
    restarted, and when it has been powered off.

    Note that interrupts will not be enabled at the time that this is called.
    They will be enabled after this callback completes.

    This function is not marked pageable because this function is in the
    device power up path. When a function is marked pagable and the code
    section is paged out, it will generate a page fault which could impact
    the fast resume behavior because the client driver will have to wait
    until the system drivers can service this page fault.

Arguments:

    Device  - The handle to the WDF device object

    PreviousState - The state the device was in before this callback was invoked.

Return Value:

    NTSTATUS

    Success implies that the device can be used.

    Failure will result in the    device stack being torn down.

--*/
{
    PINPOUTNG_CONTEXT   devContext;
    NTSTATUS          status=STATUS_SUCCESS;

    UNREFERENCED_PARAMETER(PreviousState);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "--> %!FUNC!");

    devContext = inpOutNgGetContext(Device);

    status = inpOutNgInitWrite(devContext);
    if (NT_SUCCESS(status)) {

        status = inpOutNgInitRead(devContext);

    }
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "<-- %!FUNC!");

    return status;
}

NTSTATUS
inpOutNgEvtDeviceD0Exit(
    _In_  WDFDEVICE Device,
    _In_  WDF_POWER_DEVICE_STATE TargetState
)
/*++

Routine Description:

    This routine undoes anything done in InpOutNgEvtDeviceD0Entry.  It is called
    whenever the device leaves the D0 state, which happens when the device
    is stopped, when it is removed, and when it is powered off.

    The device is still in D0 when this callback is invoked, which means that
    the driver can still touch hardware in this routine.

    Note that interrupts have already been disabled by the time that this
    callback is invoked.

Arguments:

    Device  - The handle to the WDF device object

    TargetState - The state the device will go to when this callback completes.

Return Value:

    Success implies that the device can be used.  Failure will result in the
    device stack being torn down.

--*/
{
    PINPOUTNG_CONTEXT   devContext;

    PAGED_CODE();

    devContext = inpOutNgGetContext(Device);

    switch (TargetState) {
    case WdfPowerDeviceD1:
    case WdfPowerDeviceD2:
    case WdfPowerDeviceD3:

        //
        // Fill in any code to save hardware state here.
        //

        //
        // Fill in any code to put the device in a low-power state here.
        //
        break;

    case WdfPowerDevicePrepareForHibernation:

        //
        // Fill in any code to save hardware state here.  Do not put in any
        // code to shut the device off.  If this device cannot support being
        // in the paging path (or being a parent or grandparent of a paging
        // path device) then this whole case can be deleted.
        //

        break;

    case WdfPowerDeviceD3Final:
    default:

        //
        // Reset the hardware, as we're shutting down for the last time.
        //
        inpOutNgShutdown(devContext);
        break;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
inpOutNgSetIdleAndWakeSettings(
    _In_ PINPOUTNG_CONTEXT devContext
)
/*++
Routine Description:

    Called by EvtDeviceAdd to set the idle and wait-wake policy. Registering this policy
    causes Power Management Tab to show up in the device manager. By default these
    options are enabled and the user is provided control to change the settings.

Return Value:

    NTSTATUS - Failure status is returned if the device is not capable of suspending
    or wait-waking the machine by an external event. Framework checks the
    capability information reported by the bus driver to decide whether the device is
    capable of waking the machine.

--*/
{
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS wakeSettings;
    NTSTATUS    status = STATUS_SUCCESS;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "--> %!FUNC!");

    //
    // Init the idle policy structure.
    //
    WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, IdleCanWakeFromS0);
    idleSettings.IdleTimeout = 10000; // 10-sec

    status = WdfDeviceAssignS0IdleSettings(devContext->Device, &idleSettings);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_INIT,
            "DeviceSetPowerPolicyS0IdlePolicy failed %!STATUS!", status);
        return status;
    }

    //
    // Init wait-wake policy structure.
    //
    WDF_DEVICE_POWER_POLICY_WAKE_SETTINGS_INIT(&wakeSettings);

    status = WdfDeviceAssignSxWakeSettings(devContext->Device, &wakeSettings);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_INIT,
            "DeviceAssignSxWakeSettings failed %!STATUS!", status);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "<-- InpOutNgSetIdleAndWakeSettings");

    return status;
}

NTSTATUS
inpOutNgPrepareHardware(
    _In_ PINPOUTNG_CONTEXT devContext,
    _In_ WDFCMRESLIST    ResourcesTranslated
)
/*++
Routine Description:

    Gets the HW resources assigned by the bus driver from the start-irp
    and maps it to system address space.

Arguments:

    devContext      Pointer to our DEVICE_EXTENSION

Return Value:

     None

--*/
{
    UNREFERENCED_PARAMETER(devContext);
    ULONG               i;
    NTSTATUS            status = STATUS_SUCCESS;
    CHAR*               bar;
    BOOLEAN             foundPort = FALSE;
    BOOLEAN             foundIrq  = FALSE;

    PCM_PARTIAL_RESOURCE_DESCRIPTOR  desc;

    PAGED_CODE();

    //
    // Parse the resource list and save the resource information.
    //
    for (i = 0; i < WdfCmResourceListGetCount(ResourcesTranslated); i++) {

        desc = WdfCmResourceListGetDescriptor(ResourcesTranslated, i);

        if (!desc) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONFIG,
                        "WdfResourceCmGetDescriptor failed");
            return STATUS_DEVICE_CONFIGURATION_ERROR;
        }

        switch (desc->Type) {
            case CmResourceTypeNull: {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                    " - Generic (Null) Resource [%I64X-%I64X]",
                    desc->u.Generic.Start.QuadPart,
                    desc->u.Generic.Start.QuadPart +
                    desc->u.Generic.Length);

                break;
            }
            case CmResourceTypePort: {
                bar = NULL;

                if (!foundPort &&
                    desc->u.Port.Length >= 0x10) {
                    foundPort = TRUE;
                    bar = "ISA IO Ports";
                }

                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                    " - Port   Resource [%08I64X-%08I64X] %s",
                    desc->u.Port.Start.QuadPart,
                    desc->u.Port.Start.QuadPart +
                    desc->u.Port.Length,
                    (bar) ? bar : "<unrecognized>");
                break;
            }
            case CmResourceTypeInterrupt: {
                bar = NULL;

                if (!foundIrq &&
                    desc->u.Interrupt.Vector > 0x0) {
                    foundIrq = TRUE;
                    bar = "IRQ";
                }

                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                    " - Interrupt Resource %lx, lvl %d, vector %d %s",
                    (ULONG)desc->u.Interrupt.Affinity,
                    desc->u.Interrupt.Level,
                    desc->u.Interrupt.Vector,
                    (bar) ? bar : "<unrecognized>");

                break;
            }
            case CmResourceTypeMemory: {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                    " - Memory Resource [%I64X-%I64X]",
                    desc->u.Memory.Start.QuadPart,
                    desc->u.Memory.Start.QuadPart +
                    desc->u.Memory.Length);
                break;
            }
            case CmResourceTypeDma :
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                    " - DMA Resource Channel %d Port %d [ %s ]",
                    desc->u.Dma.Channel,
                    desc->u.Dma.Port,
                    desc->u.Dma.Reserved1 ? "reserved" : "free");
                break;

            case CmResourceTypeDeviceSpecific: {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                    " - Device specific Resource [%I64X-%I64X]",
                    desc->u.Generic.Start.QuadPart,
                    desc->u.Generic.Start.QuadPart +
                    desc->u.Generic.Length);
                break;
            }
            case CmResourceTypeBusNumber: {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                    " - Bus number Resource [%I64X-%I64X]",
                    desc->u.BusNumber.Start,
                    (UINT64)((UINT64)desc->u.BusNumber.Start +
                    (UINT64)desc->u.BusNumber.Length));
                break;
            }
            case CmResourceTypeMemoryLarge: {
                TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                    " - LargeMemory Resource [%I64X-%I64X]",
                    desc->u.Memory64.Start.QuadPart,
                    desc->u.Memory64.Start.QuadPart +
                    desc->u.Memory64.Length64);
                break;
            }
            case CmResourceTypeNonArbitrated: {
                switch (desc->Type) {
                    case CmResourceTypeConfigData: {
                        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                            " - Resource Type Config");
                        break;
                    }
                    case CmResourceTypeDevicePrivate: {
                        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                            " - Device Private Config");
                        break;
                    }
                    case CmResourceTypePcCardConfig: {
                        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                            " - Device Private Config");
                        break;
                    }
                    case CmResourceTypeMfCardConfig: {
                        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                            " - Device Private Config");
                        break;
                    }
                    case CmResourceTypeConnection: {
                        TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_CONFIG,
                            " - Device Private Config");
                        break;
                    }
                    default: {
                        //
                        // Report all other descriptors
                        //
                        TraceEvents(TRACE_LEVEL_WARNING, TRACE_CONFIG,
                            " - Unknown extended config type 0x%x", desc->Type);

                        break;
                    }
                }
            }
            default: {
                //
                // Report all other descriptors
                //
                TraceEvents(TRACE_LEVEL_WARNING, TRACE_CONFIG,
                    " - Unknown config type 0x%x", desc->Type);

                break;
            }
        }
    }

    if (!(foundPort || foundIrq)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_CONFIG,
            "inpOutNgMapResources: Missing resources");
        //return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    return status;
}

NTSTATUS
inpOutNgInitWrite(
    _In_ PINPOUTNG_CONTEXT devContext
)
/*++
Routine Description:

    Initialize write data structures

Arguments:

    devContext     Pointer to Device Extension

Return Value:

    None

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "--> %!FUNC!");

    devContext->WriteReady = TRUE;


    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "<-- %!FUNC!");

    return STATUS_SUCCESS;
}


NTSTATUS
inpOutNgInitRead(
    _In_ PINPOUTNG_CONTEXT devContext
)
/*++
Routine Description:

    Initialize read data structures

Arguments:

    devContext     Pointer to Device Extension

Return Value:

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "--> %!FUNC!");

    devContext->ReadReady = TRUE;


    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "<-- %!FUNC!");

    return STATUS_SUCCESS;
}

VOID
inpOutNgShutdown(
    _In_ PINPOUTNG_CONTEXT devContext
)
/*++

Routine Description:

    Reset the device to put the device in a known initial state.
    This is called from D0Exit when the device is torn down or
    when the system is shutdown. Note that Wdf has already
    called out EvtDisable callback to disable the interrupt.

Arguments:

    devContext -  Pointer to our adapter

Return Value:

    None

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "---> %!FUNC!");

    //
    // WdfInterrupt is already disabled so issue a full reset
    //
    if (devContext) {

        inpOutNgHardwareReset(devContext);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "<--- %!FUNC!");
}

VOID
inpOutNgHardwareReset(
    _In_ PINPOUTNG_CONTEXT devContext
)
/*++
Routine Description:

    Called by D0Exit when the device is being disabled or when the system is shutdown to
    put the device in a known initial state.

Arguments:

    devContext     Pointer to Device Extension

Return Value:

--*/
{
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "--> %!FUNC!");

    //
    // Clear readiness flags.
    //
    devContext->ReadReady  = FALSE;
    devContext->WriteReady = FALSE;
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT, "<-- %!FUNC!");
}

VOID
inpOutNgEvtDeviceCleanup(
    _In_ WDFOBJECT Device
)
/*++

Routine Description:

    Invoked before the device object is deleted.

Arguments:

    Device - A handle to the WDFDEVICE

Return Value:

     None

--*/
{
    PINPOUTNG_CONTEXT devContext;

    devContext = inpOutNgGetContext((WDFDEVICE)Device);

    inpOutNgCleanupDeviceContext(devContext);
}

VOID
doTask(
    _In_ PINPOUTNG_CONTEXT devContext,
    _In_ p_port_task_t taskItemIn,
    _Out_ p_port_task_t taskItemOut
)
{
    if (!taskItemIn || !taskItemOut || !devContext)
    {
        return;
    }
    switch (taskItemIn->taskOperation) {
        case INPOUT_READ8: {
            taskItemOut->taskOperation = INPOUT_READ8_ACK;
            taskItemOut->taskData = __inbyte(taskItemIn->taskPort);
            break;
        }
        case INPOUT_WRITE8: {
            taskItemOut->taskOperation = INPOUT_WRITE8_ACK;
            __outbyte(taskItemIn->taskPort, (BYTE)(taskItemIn->taskData & 0x000000ff));
            taskItemOut->taskData = 0x0;
            break;
        }
        case INPOUT_READ16: {
            taskItemOut->taskOperation = INPOUT_READ16_ACK;
            taskItemOut->taskData = __inword(taskItemIn->taskPort);
            break;
        }
        case INPOUT_WRITE16: {
            taskItemOut->taskOperation = INPOUT_WRITE16_ACK;
            __outword(taskItemIn->taskPort, (USHORT)(taskItemIn->taskData & 0x0000ffff));
            taskItemOut->taskData = 0x0;
            break;
        }
        case INPOUT_READ32: {
            taskItemOut->taskOperation = INPOUT_READ32_ACK;
            taskItemOut->taskData = __indword(taskItemIn->taskPort);
            break;
        }
        case INPOUT_WRITE32: {
            taskItemOut->taskOperation = INPOUT_WRITE32_ACK;
            __outdword(taskItemIn->taskPort, taskItemIn->taskData);
            taskItemOut->taskData = 0x0;
            break;
        }
        default: {
            taskItemOut->taskOperation = INPOUT_IRQ_OCCURRED;
            taskItemOut->taskPort = 0xaa;
            taskItemOut->taskData = devContext->InterruptCount;
            break;
        }
    }
    return;
}
