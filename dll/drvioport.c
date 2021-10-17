#include "StdAfx.h"
#include "datadll.h"

static
ULONG
DlPortRead(
    _In_ DWORD ctlCode,
    _In_ DWORD inSize,
    _In_ USHORT portAddr
)
{
    UNREFERENCED_PARAMETER(inSize);
    __declspec(align(8)) outPortData_t outData = { .addr = 0x0, .val.outLong = 0x0 };
    __declspec(align(8)) inPortData_t inData = { .val.inLong = 0x0 };
    DWORD    opCode;
    DWORD    szReturned;

    outData.addr = (USHORT)portAddr;

    opCode = DeviceIoControl(drvHandle,
        ctlCode,
        &outData,
        sizeof(outPortData_t),
        &inData,
        sizeof(inPortData_t),
        &szReturned,
        NULL);

    if (!opCode)
    {
        msg(M_WARN | M_ERRNO, L"PortReadError");
    }

    return inData.val.inLong;
}

BOOL
WINAPI
setIrqClearSeq(
    _In_    p_port_task_t inTask,
    _In_    DWORD inTaskSize
)
{
    DWORD ioCode = ERROR_SUCCESS;
    DWORD szReturned = 0x0;
    DeviceIoControl(drvHandle,
        (DWORD)IOCTL_SET_IRQCLEAR,
        inTask,
        inTaskSize,
        NULL,
        0,
        &szReturned,
        NULL);
    ioCode = GetLastError();
    return (ioCode == ERROR_SUCCESS) ? TRUE : FALSE;
}


UCHAR
WINAPI
inp8 (
    _In_ USHORT portAddr
)
{
    return (UCHAR)(DlPortRead((DWORD)IOCTL_READ_PORT_UCHAR, sizeof(UCHAR), portAddr) & 0x000000ff);
}

USHORT
WINAPI
inp16 (
    _In_ USHORT portAddr
)
{
    return (USHORT)(DlPortRead((DWORD)IOCTL_READ_PORT_USHORT, sizeof(USHORT), portAddr) & 0x0000ffff);
}

ULONG
WINAPI
inp32 (
    _In_ USHORT portAddr
)
{
    return (ULONG)DlPortRead((DWORD)IOCTL_READ_PORT_ULONG, sizeof(USHORT), portAddr);
}

ULONG
WINAPI
irqCount (
    VOID
)
{
    return (ULONG)DlPortRead((DWORD)IOCTL_GET_IRQCOUNT, sizeof(ULONG), 0);
}

static
VOID
DlPortWrite (
    _In_ DWORD ctlCode,
    _In_ DWORD dataSize,
    _In_ USHORT portAddr,
    _In_ ULONG portData
)
{
    UNREFERENCED_PARAMETER(dataSize);
    __declspec(align(8)) outPortData_t outData = { .addr = 0x0, .val.outLong = 0x0 };
    BOOL    opCode;
    DWORD    szReturned;

    outData.addr = portAddr;
    outData.val.outLong = portData;

    opCode = DeviceIoControl(drvHandle,
        ctlCode,
        &outData,
        sizeof(outPortData_t),
        NULL,
        0,
        &szReturned,
        NULL);

    if (!opCode)
    {
        msg(M_WARN | M_ERRNO, L"PortWriteError");
    }
}

VOID
WINAPI
outp8 (
    _In_ USHORT portAddr,
    _In_ UCHAR portData
)
{
    DlPortWrite((DWORD)IOCTL_WRITE_PORT_UCHAR, sizeof(UCHAR), portAddr, (ULONG)(portData & 0x000000ff));
}

VOID
WINAPI
outp16 (
    _In_ USHORT portAddr,
    _In_  USHORT portData
)
{
    DlPortWrite((DWORD)IOCTL_WRITE_PORT_USHORT, sizeof(USHORT), portAddr, (ULONG)(portData & 0x0000ffff));
}

VOID
WINAPI
outp32 (
    _In_ USHORT portAddr,
    _In_  ULONG portData
)
{
    DlPortWrite((DWORD)IOCTL_WRITE_PORT_ULONG, sizeof(ULONG), portAddr, portData);
}