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
#pragma alloc_text (PAGE, inpoutngCreateDevice)
#endif

NTSTATUS
inpoutngCreateDevice(
    _Inout_ PWDFDEVICE_INIT DeviceInit
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
    WDF_OBJECT_ATTRIBUTES deviceAttributes;
    PDEVICE_CONTEXT deviceContext;
    WDFDEVICE device;
    NTSTATUS status;

//!!    PDEVICE_OBJECT deviceObject;
//!!    NTSTATUS status;

#ifdef _AMD64_
    WCHAR NameBuffer[] = L"\\Device\\inpoutng";//!! x64";
    WCHAR DOSNameBuffer[] = L"\\DosDevices\\inpoutng";//!! x64";
#else
    WCHAR NameBuffer[] = L"\\Device\\inpoutng";
    WCHAR DOSNameBuffer[] = L"\\DosDevices\\inpoutng";
#endif
    UNICODE_STRING uniNameString, uniDOSString;

#if 0


    status = IoCreateDevice(DriverObject,
        0,
        &uniNameString,
        FILE_DEVICE_UNKNOWN,
        0,
        FALSE,
        &deviceObject);

    if (!NT_SUCCESS(status))
        return status;

    DriverObject->MajorFunction[IRP_MJ_CREATE] = hwinterfaceCreateDispatch;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = hwinterfaceDeviceControl;
    DriverObject->DriverUnload = hwinterfaceUnload;

    return STATUS_SUCCESS;
#endif

    PAGED_CODE();

    //
    // Initialize WPP Tracing
    //
    TraceEvents(TRACE_LEVEL_INFORMATION, TRACE_DEVICE, "%!FUNC! Entry");

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &device);

    if (NT_SUCCESS(status)) {

        RtlInitUnicodeString(&uniNameString, NameBuffer);
        RtlInitUnicodeString(&uniDOSString, DOSNameBuffer);

        //
        // Get a pointer to the device context structure that we just associated
        // with the device object. We define this structure in the device.h
        // header file. DeviceGetContext is an inline function generated by
        // using the WDF_DECLARE_CONTEXT_TYPE_WITH_NAME macro in device.h.
        // This function will do the type checking and return the device context.
        // If you pass a wrong object handle it will return NULL and assert if
        // run under framework verifier mode.
        //
        deviceContext = DeviceGetContext(device);

        //
        // Initialize the context.
        //
        deviceContext->PrivateDeviceData = 0;

        //
        // Create a device interface so that applications can find and talk
        // to us.
        //
        status = WdfDeviceCreateDeviceInterface(
            device,
            &GUID_DEVINTERFACE_inpoutng,
            NULL // ReferenceString
            );

        if (NT_SUCCESS(status)) {
            //
            // Initialize the I/O Package and any Queues
            //
            status = inpoutngQueueInitialize(device);
        }
        /*
        status = WdfDeviceCreateSymbolicLink(device, &uniDOSString);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDeviceCreate DOS name creation failed %!STATUS!", status);
        }
        */
        status = WdfDeviceCreateSymbolicLink(device, &uniNameString);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, TRACE_DEVICE, "WdfDeviceCreate NT name creation failed %!STATUS!", status);
        }
    }

    return status;
}
