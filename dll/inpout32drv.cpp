/***********************************************************************\
*                                                                       *
* InpOut32drv.cpp                                                       *
*                                                                       *
* The entry point for the InpOut DLL                                    *
* Provides the 32 and 64bit implementation of InpOut32 DLL to install   *
* and call the appropriate driver and write directly to hardware ports. *
*                                                                       *
* Written by Phillip Gibbons (Highrez.co.uk)                            *
* Based on orriginal, written by Logix4U (www.logix4u.net)              *
*                                                                       *
\***********************************************************************/

#include "stdafx.h"
#include <stdlib.h>
#include <stdio.h>

#include <winioctl.h>
#include <public.h>
#include "resource.h"

BOOL	_stdcall IsXP64Bit();
BOOL	DisableWOW64(PVOID* oldValue);
BOOL	RevertWOW64(PVOID* oldValue);

int inst32();
int inst64();
int start(LPCTSTR pszDriver);

//First, lets set the DRIVERNAME depending on our configuraiton.
//!!#define DRIVERNAMEx64 _T("inpoutng64\0")
#define DRIVERNAMEx64 _T("inpoutng\0")
#define DRIVERNAMEi386 _T("inpoutng\0")

#define ARRAY_SIZE(x) ( sizeof(x) / sizeof(x[0]) )

char str[10];
int vv;

HANDLE hdriver=NULL;
TCHAR path[MAX_PATH];
HINSTANCE hmodule;
SECURITY_ATTRIBUTES sa;

int Opendriver(BOOL bX64);
void Closedriver(void);

BOOL APIENTRY DllMain( HINSTANCE  hModule, 
					  DWORD  ul_reason_for_call, 
					  LPVOID lpReserved
					  )
{
	UNREFERENCED_PARAMETER(lpReserved);
	hmodule = hModule;
	BOOL bx64 = IsXP64Bit();
	switch(ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		Opendriver(bx64);
		break;
	case DLL_PROCESS_DETACH:
		Closedriver();
		break;
	}
	return TRUE;
}

/***********************************************************************/

void Closedriver(void)
{
	if (hdriver)
	{
		OutputDebugString(_T("Closing InpOut driver...\n"));
		CloseHandle(hdriver);
		hdriver=NULL;
	}
}

void _stdcall Out32(short PortAddress, short data)
{
	UINT	error;
	DWORD	BytesReturned;
	BYTE	Buffer[3] = {NULL};
	DWORD	szBuffer  = ARRAY_SIZE(Buffer);
	PUSHORT pBuffer;

	pBuffer = (PUSHORT)&Buffer[0];
	*pBuffer = LOWORD(PortAddress);
	Buffer[2] = LOBYTE(data);

	error = DeviceIoControl(hdriver,
		DWORD(IOCTL_WRITE_PORT_UCHAR),
		&Buffer,
		szBuffer,
		NULL,
		(DWORD)0U,
		&BytesReturned,
		nullptr);
}

/*********************************************************************/

short _stdcall Inp32(short PortAddress)
{
	UINT error;
	DWORD BytesReturned;
	UCHAR Buffer[3]={NULL};
	PUSHORT pBuffer;
	pBuffer = (PUSHORT)&Buffer[0];

	*pBuffer = LOWORD(PortAddress);
	Buffer[2] = 0;

	error = DeviceIoControl(hdriver,
		DWORD(IOCTL_READ_PORT_UCHAR),
		&Buffer,
		sizeof(USHORT),
		&Buffer,
		sizeof(UCHAR),
		&BytesReturned,
		nullptr);

	if (error==0)
	{
		DWORD dwError = GetLastError();
		TCHAR tszError[255];
		_stprintf_s(tszError, 255, _T("Error %u\n"), dwError);
		OutputDebugString(tszError);
	}

	//Do this to ensure only the first byte is returned, we dont really want to return a short as were only reading a byte.
	//but we also dont want to change the InpOut interface!
	UCHAR ucRes = (UCHAR)Buffer[0];
	return ucRes;
}

/*********************************************************************/

int Opendriver(BOOL bX64)
{
	OutputDebugString(_T("Whoopeee... Attempting to open InpOut driver...\n"));
	TCHAR szFileName[MAX_PATH] = {NULL};
	_stprintf_s(szFileName, MAX_PATH, _T("\\\\.\\GLOBALROOT\\Device\\%s"), bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);// ("\\\\.\\Device\\%s"), bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);

	hdriver = CreateFile(szFileName, 
		GENERIC_READ | GENERIC_WRITE, 
		0, 
		NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if(hdriver == INVALID_HANDLE_VALUE) 
	{
		if(start(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386))
		{
			/*
			if (bX64)
				inst64();	//Install the x64 driver
			else
				inst32();	//Install the i386 driver
				*/
			int nResult = start(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);

			if (nResult == ERROR_SUCCESS)
			{
				hdriver = CreateFile(szFileName, 
					GENERIC_READ | GENERIC_WRITE, 
					0, 
					NULL,
					OPEN_EXISTING, 
					FILE_ATTRIBUTE_NORMAL, 
					NULL);

				if(hdriver != INVALID_HANDLE_VALUE) 
				{
					OutputDebugString(_T("Successfully opened "));
					OutputDebugString(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);
					OutputDebugString(_T(" driver\n"));
					return 0;
				}
			}
			else
			{
				TCHAR szMsg[MAX_PATH] = {NULL};
				_stprintf_s(szMsg, MAX_PATH, _T("Unable to open %s driver. Error code %d.\n"), bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386, nResult);
				OutputDebugString(szMsg);

				//RemoveDriver();
			}
		}
		return 1;
	}

	OutputDebugString(_T("Successfully opened "));
	OutputDebugString(bX64 ? DRIVERNAMEx64 : DRIVERNAMEi386);
	OutputDebugString(_T(" driver\n"));
	return 0;
}

/***********************************************************************/
int inst32()
{
	TCHAR szDriverSys[MAX_PATH];

	int errCode = ERROR_SUCCESS;

	SC_HANDLE Mgr = NULL;
	SC_HANDLE Ser = NULL;

	_tcscpy_s(szDriverSys, MAX_PATH, DRIVERNAMEi386);
	_tcscat_s(szDriverSys, MAX_PATH, _T(".sys\0"));

	GetSystemDirectory(path, MAX_PATH);
	HRSRC hResource = FindResource(hmodule, MAKEINTRESOURCE(IDR_INPOUT32), _T("bin"));
	if(hResource)
	{
		HGLOBAL binGlob = LoadResource(hmodule, hResource);

		if(binGlob)
		{
			void *binData = LockResource(binGlob);

			if(binData)
			{
				HANDLE file;
				
				_tcscat_s(path, sizeof(path), _T("\\Drivers\\"));
				_tcscat_s(path, sizeof(path), szDriverSys);
			
				file = CreateFile(path,
					GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					0,
					NULL);

				if (file && file != INVALID_HANDLE_VALUE)
				{
					DWORD size, written;
					size = SizeofResource(hmodule, hResource);
					WriteFile(file, binData, size, &written, NULL);
					CloseHandle(file);
				}
				else
				{
					//Error
				}
			}
		}
	}

	Mgr = OpenSCManager (NULL, NULL,SC_MANAGER_ALL_ACCESS);
	if (Mgr == NULL)
	{							//No permission to create service
		if ( (errCode=GetLastError()) == ERROR_ACCESS_DENIED ) 
		{
			return errCode;  // error access denied
		}
	}	
	else
	{
		TCHAR szFullPath[MAX_PATH] = _T("System32\\Drivers\\\0");
		_tcscat_s(szFullPath, MAX_PATH, szDriverSys);
		Ser = CreateService (Mgr,                      
			DRIVERNAMEi386,                        
			DRIVERNAMEi386,                        
			SERVICE_ALL_ACCESS,                
			SERVICE_KERNEL_DRIVER,             
			SERVICE_AUTO_START,               
			SERVICE_ERROR_NORMAL,               
			szFullPath,  
			NULL,                               
			NULL,                              
			NULL,                               
			NULL,                              
			NULL                               
			);
	}
	CloseServiceHandle(Ser);
	CloseServiceHandle(Mgr);

	return 0;
}

int inst64()
{
	TCHAR szDriverSys[MAX_PATH];
	
	SC_HANDLE  Mgr = NULL;
	SC_HANDLE  Ser = NULL;

	_tcscpy_s(szDriverSys, MAX_PATH, DRIVERNAMEx64);
	_tcscat_s(szDriverSys, MAX_PATH, _T(".sys\0"));

	GetSystemDirectory(path, MAX_PATH);
	HRSRC hResource = FindResource(hmodule, MAKEINTRESOURCE(IDR_INPOUTX64), _T("bin"));

	if(hResource)
	{
		HGLOBAL binGlob = LoadResource(hmodule, hResource);

		if(binGlob)
		{
			void *binData = LockResource(binGlob);

			if(binData)
			{
				HANDLE file;
				_tcscat_s(path, sizeof(path), _T("\\Drivers\\"));
				_tcscat_s(path, sizeof(path), szDriverSys);
			
				PVOID OldValue;
				DisableWOW64(&OldValue);
				file = CreateFile(path,
					GENERIC_WRITE,
					0,
					NULL,
					CREATE_ALWAYS,
					0,
					NULL);

				if(file && file != INVALID_HANDLE_VALUE)
				{
					DWORD size, written;

					size = SizeofResource(hmodule, hResource);
					WriteFile(file, binData, size, &written, NULL);
					CloseHandle(file);
				}
				else
				{
					//error
				}
				RevertWOW64(&OldValue);
			}
		}
	}

	Mgr = OpenSCManager (NULL, NULL,SC_MANAGER_ALL_ACCESS);
	if (Mgr == NULL)
	{							//No permission to create service
		if (GetLastError() == ERROR_ACCESS_DENIED) 
		{
			return 5;  // error access denied
		}
	}	
	else
	{
		TCHAR szFullPath[MAX_PATH] = _T("System32\\Drivers\\\0");
		_tcscat_s(szFullPath, MAX_PATH, szDriverSys);
		Ser = CreateService (Mgr,                      
			DRIVERNAMEx64,                        
			DRIVERNAMEx64,                        
			SERVICE_ALL_ACCESS,                
			SERVICE_KERNEL_DRIVER,             
			SERVICE_AUTO_START,               
			SERVICE_ERROR_NORMAL,               
			szFullPath,  
			NULL,                               
			NULL,                              
			NULL,                               
			NULL,                              
			NULL                               
			);
	}
	CloseServiceHandle(Ser);
	CloseServiceHandle(Mgr);

	return 0;
}

/**************************************************************************/
int start(LPCTSTR pszDriver)
{
	SC_HANDLE  Mgr;
	SC_HANDLE  Ser;

	Mgr = OpenSCManager (NULL, NULL,SC_MANAGER_ALL_ACCESS);

	if (Mgr == NULL)
	{							//No permission to create service
		if (GetLastError() == ERROR_ACCESS_DENIED) 
		{
			Mgr = OpenSCManager (NULL, NULL, GENERIC_READ);
			Ser = OpenService(Mgr, pszDriver, GENERIC_EXECUTE);
			if (Ser)
			{    // we have permission to start the service
				if(!StartService(Ser,0,NULL))
				{
					CloseServiceHandle (Ser);
					return ERROR_SERVICE_NOT_ACTIVE; // we could open the service but unable to start
				}
			}
		}
	}
	else
	{// Successfuly opened Service Manager with full access
		Ser = OpenService(Mgr, pszDriver, SERVICE_ALL_ACCESS);
		if (Ser)
		{
			if(!StartService(Ser,0,NULL))
			{
				CloseServiceHandle (Ser);
				return ERROR_SERVICE_START_HANG; // opened the Service handle with full access permission, but unable to start
			}
			else
			{
				CloseServiceHandle (Ser);
				return ERROR_SUCCESS;
			}
		}
	}
	return 1;
}

BOOL _stdcall IsInpOutDriverOpen()
{
	return (hdriver != INVALID_HANDLE_VALUE && hdriver != NULL) ? TRUE : FALSE;
}

UCHAR _stdcall DlPortReadPortUchar (USHORT port)
{
	UINT error;
	DWORD BytesReturned;
	BYTE Buffer[3]={NULL};
	PUSHORT pBuffer;
	pBuffer = (PUSHORT)&Buffer[0];
	*pBuffer = port;
	Buffer[2] = 0;
	error = DeviceIoControl(hdriver,
		DWORD(IOCTL_READ_PORT_UCHAR),
		&Buffer,
		sizeof(Buffer),
		&Buffer,
		sizeof(Buffer),
		&BytesReturned,
		nullptr);

	return((UCHAR)Buffer[0]);
}

void _stdcall DlPortWritePortUchar (USHORT port, UCHAR Value)
{
	UINT error;
	DWORD BytesReturned;        
	BYTE Buffer[3]={NULL};
	PUSHORT pBuffer;
	pBuffer = (PUSHORT)&Buffer[0];
	*pBuffer = port;
	Buffer[2] = Value;

	error = DeviceIoControl(hdriver,
		DWORD(IOCTL_WRITE_PORT_UCHAR),
		&Buffer,
		sizeof(Buffer),
		nullptr,
		0,
		&BytesReturned,
		nullptr);
}

USHORT _stdcall DlPortReadPortUshort (USHORT port)
{
	UINT error;
	DWORD BytesReturned;
	USHORT sPort=port;
	error = DeviceIoControl(hdriver,
		DWORD(IOCTL_READ_PORT_USHORT),
		&sPort,
		sizeof(unsigned short),
		&sPort,
		sizeof(unsigned short),
		&BytesReturned,
		nullptr);
	return(sPort);
}

void _stdcall DlPortWritePortUshort (USHORT port, USHORT Value)
{
	UINT error;
	DWORD BytesReturned;        
	BYTE Buffer[5]={NULL};
	PUSHORT pBuffer;
	pBuffer = (PUSHORT)&Buffer[0];
	*pBuffer = LOWORD(port);
	*(pBuffer+1) = Value;

	error = DeviceIoControl(hdriver,
		DWORD(IOCTL_WRITE_PORT_USHORT),
		&Buffer,
		sizeof(Buffer),
		nullptr,
		0,
		&BytesReturned,
		nullptr);
}

ULONG _stdcall DlPortReadPortUlong(ULONG port)
{
	UINT error;
	DWORD BytesReturned;
	ULONG lPort=port;

    PULONG  ulBuffer;
    ulBuffer = (PULONG)&lPort;

	error = DeviceIoControl(hdriver,
		DWORD(IOCTL_READ_PORT_ULONG),
		&lPort,
		sizeof(unsigned long),
		&lPort,
		sizeof(unsigned long),
		&BytesReturned,
		nullptr);
	return(lPort);
}

void _stdcall DlPortWritePortUlong (ULONG port, ULONG Value)
{
	UINT error;
	DWORD BytesReturned;        
	BYTE Buffer[8] = {NULL};
	PULONG pBuffer;
	pBuffer = (PULONG)&Buffer[0];
	*pBuffer = port;
	*(pBuffer+1) = Value;

	error = DeviceIoControl(hdriver,
		DWORD(IOCTL_WRITE_PORT_ULONG),
		&Buffer,
		sizeof(Buffer),
		nullptr,
		0U,
		&BytesReturned,
		nullptr);
}


// Support functions for WinIO
PBYTE _stdcall MapPhysToLin(PBYTE pbPhysAddr, DWORD dwPhysSize, HANDLE *pPhysicalMemoryHandle)
{
	PBYTE pbLinAddr;
	tagPhys32Struct Phys32Struct;
	DWORD dwBytesReturned;

	Phys32Struct.dwPhysMemSizeInBytes = dwPhysSize;
	Phys32Struct.pvPhysAddress = pbPhysAddr;

	if (!DeviceIoControl(hdriver, DWORD(IOCTL_WINIO_MAPPHYSTOLIN), &Phys32Struct,
		sizeof(tagPhys32Struct), &Phys32Struct, sizeof(tagPhys32Struct),
		&dwBytesReturned, nullptr))
		return nullptr;
	else
	{
#ifdef _M_X64
		pbLinAddr = (PBYTE)((LONGLONG)Phys32Struct.pvPhysMemLin + (LONGLONG)pbPhysAddr - (LONGLONG)Phys32Struct.pvPhysAddress);
#else
		pbLinAddr = (PBYTE)((DWORD)Phys32Struct.pvPhysMemLin + (DWORD)pbPhysAddr - (DWORD)Phys32Struct.pvPhysAddress);
#endif
		*pPhysicalMemoryHandle = Phys32Struct.PhysicalMemoryHandle;
	}
	return pbLinAddr;
}


BOOL _stdcall UnmapPhysicalMemory(HANDLE PhysicalMemoryHandle, PBYTE pbLinAddr)
{
	tagPhys32Struct Phys32Struct;
	DWORD dwBytesReturned;

	Phys32Struct.PhysicalMemoryHandle = PhysicalMemoryHandle;
	Phys32Struct.pvPhysMemLin = pbLinAddr;

	if (!DeviceIoControl(hdriver, DWORD(IOCTL_WINIO_UNMAPPHYSADDR), &Phys32Struct,
		sizeof(tagPhys32Struct), nullptr, 0, &dwBytesReturned, nullptr))
		return false;

	return true;
}

BOOL _stdcall GetPhysLong(PBYTE pbPhysAddr, PDWORD pdwPhysVal)
{
	PDWORD pdwLinAddr;
	HANDLE PhysicalMemoryHandle;

	pdwLinAddr = (PDWORD)MapPhysToLin(pbPhysAddr, 4, &PhysicalMemoryHandle);
	if (pdwLinAddr == nullptr)
		return false;

	*pdwPhysVal = *pdwLinAddr;
	UnmapPhysicalMemory(PhysicalMemoryHandle, (PBYTE)pdwLinAddr);
	return true;
}


BOOL _stdcall SetPhysLong(PBYTE pbPhysAddr, DWORD dwPhysVal)
{
	PDWORD pdwLinAddr;
	HANDLE PhysicalMemoryHandle;

	pdwLinAddr = (PDWORD)MapPhysToLin(pbPhysAddr, 4, &PhysicalMemoryHandle);
	if (pdwLinAddr == NULL)
		return false;

	*pdwLinAddr = dwPhysVal;
	UnmapPhysicalMemory(PhysicalMemoryHandle, (PBYTE)pdwLinAddr);
	return true;
}
