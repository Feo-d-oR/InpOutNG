#include "StdAfx.h"
#include "datadll.h"

// Support functions for WinIO
PBYTE _stdcall MapPhysToLin(PBYTE pbPhysAddr, DWORD dwPhysSize, HANDLE* pPhysicalMemoryHandle)
{
	PBYTE pbLinAddr;
	struct tagPhys32Struct Phys32Struct;
	DWORD dwBytesReturned;

	Phys32Struct.dwPhysMemSizeInBytes = dwPhysSize;
	Phys32Struct.pvPhysAddress = pbPhysAddr;

	if (!DeviceIoControl(drvHandle, (DWORD)IOCTL_WINIO_MAPPHYSTOLIN, &Phys32Struct,
		sizeof(struct tagPhys32Struct), &Phys32Struct, sizeof(struct tagPhys32Struct),
		&dwBytesReturned, NULL))
		return NULL;
	else
	{
#ifdef _M_X64
		pbLinAddr = (PBYTE)((LONGLONG)Phys32Struct.pvPhysMemLin + (LONGLONG)pbPhysAddr - (LONGLONG)Phys32Struct.pvPhysAddress);
#else
		pbLinAddr = (PBYTE)((DWORD)Phys32Struct.pvPhysMemLin + (DWORD)pbPhysAddr - (DWORD)Phys32Struct.pvPhysAddress);
#endif
		* pPhysicalMemoryHandle = Phys32Struct.PhysicalMemoryHandle;
	}
	return pbLinAddr;
}


BOOL _stdcall UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, PBYTE pbLinAddr)
{
	struct tagPhys32Struct Phys32Struct;
	DWORD dwBytesReturned;

	Phys32Struct.PhysicalMemoryHandle = PhysicalMemoryHandle;
	Phys32Struct.pvPhysMemLin = pbLinAddr;

	if (!DeviceIoControl(drvHandle, (DWORD)IOCTL_WINIO_UNMAPPHYSADDR, &Phys32Struct,
		sizeof(struct tagPhys32Struct), NULL, 0, &dwBytesReturned, NULL))
		return FALSE;

	return TRUE;
}

BOOL _stdcall GetPhysLong(PBYTE pbPhysAddr, PDWORD pdwPhysVal)
{
	PDWORD pdwLinAddr;
	HANDLE PhysicalMemoryHandle;

	pdwLinAddr = (PDWORD)MapPhysToLin(pbPhysAddr, 4, &PhysicalMemoryHandle);
	if (pdwLinAddr == NULL)
		return FALSE;

	*pdwPhysVal = *pdwLinAddr;
	UnmapPhysicalMemory(PhysicalMemoryHandle, (PBYTE)pdwLinAddr);
	return FALSE;
}

BOOL _stdcall SetPhysLong(PBYTE pbPhysAddr, DWORD dwPhysVal)
{
	PDWORD pdwLinAddr;
	HANDLE PhysicalMemoryHandle;

	pdwLinAddr = (PDWORD)MapPhysToLin(pbPhysAddr, 4, &PhysicalMemoryHandle);
	if (pdwLinAddr == NULL)
		return FALSE;

	*pdwLinAddr = dwPhysVal;
	UnmapPhysicalMemory(PhysicalMemoryHandle, (PBYTE)pdwLinAddr);
	return TRUE;
}