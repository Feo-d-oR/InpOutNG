/***********************************************************************\
*                                                                       *
* InpOut32drv.cpp                                                       *
*                                                                       *
* The entry point for the InpOut DLL                                    *
* Provides the 32 and 64bit implementation of InpOut32 DLL to install   *
* and call the appropriate driver and write directly to hardware ports. *
*                                                                       *
* Written by															*
* - Gvozdev A. Feodor (fagvozdev@mail.ru)								*
* - Phillip Gibbons (Highrez.co.uk)										*
* Based on orriginal, written by Logix4U (www.logix4u.net),				*
* but deeply refactored													*
*                                                                       *
\***********************************************************************/

#include "stdafx.h"
#include "inpout32.h"
#include <stdlib.h>
#include <stdio.h>

#include <winioctl.h>
#include "datadll.h"

HANDLE drvHandle = INVALID_HANDLE_VALUE;
HINSTANCE dllInstance = INVALID_HANDLE_VALUE;

//SECURITY_ATTRIBUTES inpSa;

TCHAR inpPath[MAX_PATH];

BOOL APIENTRY DllMain(HINSTANCE	hinstDll, 
					  DWORD		fdwReason, 
					  LPVOID	lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);
	
	BOOL bx64 = IsXP64Bit();
	
	dllInstance = hinstDll;
	
	
	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			drvOpen(bx64);
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			drvClose();
			drvHandle = INVALID_HANDLE_VALUE;
			dllInstance = INVALID_HANDLE_VALUE;
			break;
		}
	}

	return TRUE;
}

/***********************************************************************/

void drvClose(void)
{
	if (drvHandle)
	{
		msg(M_DEBUG, L"Closing InpOut driver...");
		CloseHandle(drvHandle);
		drvHandle=NULL;
	}
}

/*********************************************************************/

int drvOpen(BOOL bX64)
{
	UNREFERENCED_PARAMETER(bX64);
	DWORD status = ERROR_FILE_NOT_FOUND;

	msg(M_DEBUG, L"Attempting to open InpOutNG driver...");
	TCHAR szFileName[MAX_PATH] = { 0x0 };
	_stprintf_s(szFileName, MAX_PATH, _T("\\\\.\\GLOBALROOT\\Device\\%s"), DRIVERNAME);
	
	drvHandle = CreateFile(szFileName, 
		GENERIC_READ | GENERIC_WRITE, 
		0, 
		NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if(isNotHandle(drvHandle))
	{
		status = drvInst();
		if (status != ERROR_SUCCESS)
		{
			msg((M_ERR | M_ERRNO), L"Unable to install %s driver. Error code %d.", DRIVERNAME, status);
		}
		else
		{
			drvHandle = CreateFile(szFileName, 
				GENERIC_READ | GENERIC_WRITE, 
				0, 
				NULL,
				OPEN_EXISTING, 
				FILE_ATTRIBUTE_NORMAL, 
				NULL);

			if (isNotHandle(drvHandle))
			{
				msg((M_WARN | M_ERRNO), L"Unable to open %s driver. Error code %d.", DRIVERNAME, status);
			}
			else
			{
				msg(M_DEBUG, L"Successfully opened %s driver\n", DRIVERNAME);
				return ERROR_SUCCESS;
			}
		}
		return ERROR_FILE_NOT_FOUND;
	}

	msg(M_DEBUG, L"Successfully opened %s driver.", DRIVERNAME);
	return 0;
}

BOOL _stdcall IsInpOutDriverOpen()
{
	return ( (drvHandle != INVALID_HANDLE_VALUE) && (drvHandle != NULL) ) ? TRUE : FALSE;
}
