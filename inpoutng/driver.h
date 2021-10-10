/*++

Module Name:

    driver.h

Abstract:

    This file contains the driver definitions.

Environment:

    Kernel-mode Driver Framework

--*/

#include <ntddk.h>
#include <wdf.h>
#include <initguid.h>

#include "device.h"
#include "queue.h"
#include "trace.h"

EXTERN_C_START

#define INPOUTNG_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#ifdef _NTDDK_
#pragma pack(push)
#pragma pack(1)
typedef struct S_PortTask
{
    USHORT taskOperation;
    USHORT taskPort;
    ULONG  taskData;
} port_task_t, * p_port_task_t;

typedef enum E_PORT_TASK
{
    INPOUT_NOTIFY       = 0x00,
    INPOUT_READ8        = 0x01,
    INPOUT_READ16       = 0x02,
    INPOUT_READ32       = 0x04,
    INPOUT_WRITE8       = 0x08,
    INPOUT_WRITE16      = 0x10,
    INPOUT_WRITE32      = 0x20,
    INPOUT_IRQ_OCCURRED = 0x40,
    INPOUT_ACK          = 0x80,
    INPOUT_READ8_ACK    = 0x81,
    INPOUT_READ16_ACK   = 0x82,
    INPOUT_READ32_ACK   = 0x84,
    INPOUT_WRITE8_ACK   = 0x88,
    INPOUT_WRITE16_ACK  = 0x90,
    INPOUT_WRITE32_ACK  = 0xa0,
    INPOUT_IRQ_EMPTY    = 0xc0
} port_operation_t, * p_port_operation_t;

typedef struct S_InPortData
{
    USHORT addr;
    union U_inPortVal
    {
        UCHAR  inChar;
        USHORT inShrt;
        ULONG  inLong;
    } val;
} inPortData_t, * p_inPortData_t;

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
#endif

//
// WDFDRIVER Events
//

DRIVER_INITIALIZE DriverEntry;
EVT_WDF_DRIVER_DEVICE_ADD inpOutNgEvtDeviceAdd;
EVT_WDF_OBJECT_CONTEXT_CLEANUP inpOutNgEvtDriverContextCleanup;

EVT_WDF_OBJECT_CONTEXT_CLEANUP inpOutNgEvtDeviceCleanup;
EVT_WDF_DEVICE_D0_ENTRY inpOutNgEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT inpOutNgEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE inpOutNgEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE inpOutNgEvtDeviceReleaseHardware;

EVT_WDF_INTERRUPT_ISR inpOutNgEvtInterruptIsr;
EVT_WDF_INTERRUPT_DPC inpOutNgEvtInterruptDpc;
EVT_WDF_INTERRUPT_ENABLE inpOutNgEvtInterruptEnable;
EVT_WDF_INTERRUPT_DISABLE inpOutNgEvtInterruptDisable;
EVT_WDF_TIMER inpOutNgOnTimer;

NTSTATUS
inpOutNgInterruptCreate(
    IN PINPOUTNG_CONTEXT DevExt
);

NTSTATUS
inpOutNgSetIdleAndWakeSettings(
    IN PINPOUTNG_CONTEXT FdoData
);

EXTERN_C_END
