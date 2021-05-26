#include "StdAfx.h"
#include "datadll.h"

#pragma pack(push)
#pragma pack(1)
typedef struct S_OutPortData
{
	USHORT addr;
	union U_outPortVal
	{
		UCHAR  outChar;
		USHORT outShrt;
		ULONG  outLong;
	} val;
} outPortData_t, * p_outPortData_t;

typedef struct S_InPortData
{
	union U_inPortVal
	{
		UCHAR  inChar;
		USHORT inShrt;
		ULONG  inLong;
	} val;
} inPortData_t, * p_inPortData_t;

#pragma pack()

static ULONG DlPortRead(DWORD ctlCode, DWORD inSize, USHORT portAddr)
{
	__declspec(align(8)) outPortData_t outData = { .addr = 0x0, .val.outLong = 0x0 };
	__declspec(align(8)) inPortData_t inData = { .val.inLong = 0x0 };
	DWORD	opCode;
	DWORD	szReturned;

	outData.addr = (USHORT)portAddr;

	opCode = DeviceIoControl(drvHandle,
		ctlCode,
		&outData,
		sizeof(outData.addr),
		&inData,
		inSize,
		&szReturned,
		NULL);

	if (!opCode)
	{
		msg(M_WARN | M_ERRNO, L"PortReadError");
	}

	return inData.val.inLong;
}

SHORT _stdcall Inp32(short portAddr)
{
	return DlPortReadPortUshort(portAddr);
}

UCHAR _stdcall DlPortReadPortUchar(USHORT portAddr)
{
	return (UCHAR)(DlPortRead((DWORD)IOCTL_READ_PORT_UCHAR, sizeof(UCHAR), portAddr) & 0x000000ff);
}

USHORT _stdcall DlPortReadPortUshort(USHORT portAddr)
{
	return (USHORT)(DlPortRead((DWORD)IOCTL_READ_PORT_USHORT, sizeof(USHORT), portAddr) & 0x0000ffff);
}

ULONG _stdcall DlPortReadPortUlong(ULONG portAddr)
{
	return (ULONG)DlPortRead((DWORD)IOCTL_READ_PORT_ULONG, sizeof(USHORT), (USHORT)portAddr);
}

static void DlPortWrite(DWORD ctlCode, DWORD dataSize, USHORT portAddr, ULONG portData)
{
	__declspec(align(8)) outPortData_t outData = { .addr = 0x0, .val.outLong = 0x0 };
	BOOL	opCode;
	DWORD	szReturned;

	outData.addr = portAddr;
	outData.val.outLong = portData;

	opCode = DeviceIoControl(drvHandle,
		ctlCode,
		&outData,
		sizeof(outData.addr) + dataSize,
		NULL,
		0,
		&szReturned,
		NULL);

	if (!opCode)
	{
		msg(M_WARN | M_ERRNO, L"PortWriteError");
	}
}

void _stdcall Out32(short portAddr, short portData)
{
	DlPortWritePortUchar(portAddr, (UCHAR)(portData & 0x00ff));
}

void _stdcall DlPortWritePortUchar(USHORT portAddr, UCHAR portData)
{
	DlPortWrite((DWORD)IOCTL_WRITE_PORT_UCHAR, sizeof(UCHAR), portAddr, (ULONG)(portData & 0x000000ff));
}

void _stdcall DlPortWritePortUshort(USHORT portAddr, USHORT portData)
{
	DlPortWrite((DWORD)IOCTL_WRITE_PORT_USHORT, sizeof(USHORT), portAddr, (ULONG)(portData & 0x0000ffff));
}

void _stdcall DlPortWritePortUlong(ULONG portAddr, ULONG portData)
{
	DlPortWrite((DWORD)IOCTL_WRITE_PORT_ULONG, sizeof(ULONG), (USHORT)(portAddr & 0x0000ffff), portData);
}