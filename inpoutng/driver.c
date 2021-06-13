/*++

Module Name:

    driver.c

Abstract:

    This file contains the driver entry points and callbacks.

Environment:

    Kernel-mode Driver Framework

--*/

#include "driver.h"
#include "driver.tmh"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, inpOutNgEvtDeviceAdd)
#pragma alloc_text (PAGE, inpOutNgEvtDriverContextCleanup)
#endif

NTSTATUS
DriverEntry(
    _In_ PDRIVER_OBJECT  DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
/*++

Routine Description:
    DriverEntry initializes the driver and is the first routine called by the
    system after the driver is loaded. DriverEntry specifies the other entry
    points in the function driver, such as EvtDevice and DriverUnload.

Parameters Description:

    DriverObject - represents the instance of the function driver that is loaded
    into memory. DriverEntry must initialize members of DriverObject before it
    returns to the caller. DriverObject is allocated by the system before the
    driver is loaded, and it is released by the system after the system unloads
    the function driver from memory.

    RegistryPath - represents the driver specific path in the Registry.
    The function driver can use the path to store driver related data between
    reboots. The path does not store hardware instance specific data.

Return Value:

    STATUS_SUCCESS if successful,
    STATUS_UNSUCCESSFUL otherwise.

--*/
{
    WDF_DRIVER_CONFIG       config;
    WDF_OBJECT_ATTRIBUTES   attributes;

    NTSTATUS status;

    //
    // Initialize WPP Tracing
    //
    WPP_INIT_TRACING(DriverObject, RegistryPath);

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Entry");

    KdPrint(("%!FUNC! Entry"));
    //
    // Register a cleanup callback so that we can call WPP_CLEANUP when
    // the framework driver object is deleted during driver unload.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.EvtCleanupCallback = inpOutNgEvtDriverContextCleanup;

    WDF_DRIVER_CONFIG_INIT(&config,
                           inpOutNgEvtDeviceAdd
                           );

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,
                             &config,
                             WDF_NO_HANDLE
                             );

    if (!NT_SUCCESS(status))
        return status;

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "WdfDriverCreate failed %!STATUS!", status);
        WPP_CLEANUP(DriverObject);
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "%!FUNC! Exit");

    return status;
}

NTSTATUS
inpOutNgEvtDeviceAdd(
    _In_    WDFDRIVER       Driver,
    _Inout_ PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:

    EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of the device.

Arguments:

    Driver - Handle to a framework driver object created in DriverEntry

    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

    NTSTATUS

--*/
{
    UNREFERENCED_PARAMETER(Driver);

    NTSTATUS                     status = STATUS_SUCCESS;
    WDF_PNPPOWER_EVENT_CALLBACKS pnpPowerCallbacks;
    WDF_OBJECT_ATTRIBUTES        attributes;
    WDFDEVICE                    device;
    PINPOUTNG_CONTEXT              devContext = NULL;

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "--> %!FUNC!");

    PAGED_CODE();

    WdfDeviceInitSetIoType(DeviceInit, WdfDeviceIoDirect);

    //
    // Zero out the PnpPowerCallbacks structure.
    //
    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    //
    // Set Callbacks for any of the functions we are interested in.
    // If no callback is set, Framework will take the default action
    // by itself.
    //
    pnpPowerCallbacks.EvtDevicePrepareHardware = inpOutNgEvtDevicePrepareHardware;
    pnpPowerCallbacks.EvtDeviceReleaseHardware = inpOutNgEvtDeviceReleaseHardware;

    //
    // These two callbacks set up and tear down hardware state that must be
    // done every time the device moves in and out of the D0-working state.
    //
    pnpPowerCallbacks.EvtDeviceD0Entry = inpOutNgEvtDeviceD0Entry;
    pnpPowerCallbacks.EvtDeviceD0Exit =  inpOutNgEvtDeviceD0Exit;

    //
    // Register the PnP Callbacks..
    //
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Initialize Fdo Attributes.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, INPOUTNG_CONTEXT);
    //
    // By opting for SynchronizationScopeDevice, we tell the framework to
    // synchronize callbacks events of all the objects directly associated
    // with the device. In this driver, we will associate queues and
    // and DpcForIsr. By doing that we don't have to worrry about synchronizing
    // access to device-context by Io Events and DpcForIsr because they would
    // not concurrently ever. Framework will serialize them by using an
    // internal device-lock.
    //
    attributes.SynchronizationScope = WdfSynchronizationScopeDevice;

    //
    // This callback is invoked when the device is removed or the driver
    // is unloaded.
    //
    attributes.EvtCleanupCallback = inpOutNgEvtDeviceCleanup;

    status = inpOutNgCreateDevice(DeviceInit, &device);

    if (!NT_SUCCESS(status))
    {
        //
        // Device Initialization failed.
        //
        TraceEvents(TRACE_LEVEL_ERROR, TRACE_DRIVER, "DeviceCreate failed %!STATUS!", status);
        return status;
    }
    //
    // Get the DeviceExtension and initialize it. inpOutNgGetContext is an inline function
    // defined by WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in the
    // private header file. This function will do the type checking and return
    // the device context. If you pass a wrong object a wrong object handle
    // it will return NULL and assert if run under framework verifier mode.
    //
    devContext = inpOutNgGetContext(device);

    devContext->Device = device;
    devContext->inpOutNgVersion = INPOUTNG_VERSION(2, 0, 1);

    TraceEvents(TRACE_LEVEL_INFORMATION,
                TRACE_DRIVER,
                "     AddDevice PDO (0x%p) FDO (0x%p), DevExt (0x%p)",
                WdfDeviceWdmGetPhysicalDevice(device),
                WdfDeviceWdmGetDeviceObject(device), devContext);

    //
    // Set the idle and wait-wake policy for this device.
    //
    status = inpOutNgSetIdleAndWakeSettings(devContext);

    if (!NT_SUCCESS(status)) {
        //
        // NOTE: The attempt to set the Idle and Wake options
        //       is a best-effort try. Failure is probably due to
        //       the non-driver environmentals, such as the system,
        //       bus or OS indicating that Wake is not supported for
        //       this case.
        //       All that being said, it probably not desirable to
        //       return the failure code as it would cause the
        //       AddDevice to fail and Idle and Wake are probably not
        //       "must-have" options.
        //
        //       You must decide for your case whether Idle/Wake are
        //       "must-have" options...but my guess is probably not.
        //
#if 1
        status = STATUS_SUCCESS;
#else
        return status;
#endif
    }

    //
    // Initalize the Device Extension.
    //
    status = inpOutNgInitializeDeviceContext(devContext);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DRIVER, "<-- %!FUNC! %!STATUS!", status);

    return status;
}

_Use_decl_annotations_
VOID
inpOutNgEvtDriverContextCleanup(
    WDFOBJECT Driver
)
/*++
Routine Description:

    Free all the resources allocated in DriverEntry.

Arguments:

    Driver - handle to a WDF Driver object.

Return Value:

    VOID.

--*/
{
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_INIT,
        "--> %!FUNC!");

    WPP_CLEANUP(WdfDriverWdmGetDriverObject(Driver));

}
