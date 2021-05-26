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

HANDLE		drvHandle	= INVALID_HANDLE_VALUE;
HINSTANCE	dllInstance = INVALID_HANDLE_VALUE;
HWND		parentHwnd	= INVALID_HANDLE_VALUE;
//SECURITY_ATTRIBUTES inpSa;

static DWORD  drvInstThreadId	= 0x0;
static HANDLE drvInstThread		= INVALID_HANDLE_VALUE;
static BOOL   bx64				= FALSE;

static drvInstState_t	drvState = DS_STARTUP;

BOOL APIENTRY DllMain(HINSTANCE	hinstDll, 
					  DWORD		fdwReason, 
					  LPVOID	lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);
	
	bx64 = IsXP64Bit();
	
	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			dllInstance = hinstDll;
			drvOpen(bx64, &drvState);
			break;
		}
		case DLL_PROCESS_DETACH:
		{
			drvClose();
			drvHandle = INVALID_HANDLE_VALUE;
			dllInstance = INVALID_HANDLE_VALUE;
			drvState = DS_STARTUP;
			break;
		}
	}
	drvState = DS_STARTED;
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

int drvOpen(BOOL bX64, p_drvInstState_t pdrvState)
{
	UNREFERENCED_PARAMETER(bX64);
	DWORD status = ERROR_FILE_NOT_FOUND;
	TCHAR szFileName[MAX_PATH] = { 0x0 };
	if (*pdrvState == DS_STARTUP)
	{
		msg(M_DEBUG, L"We are in startup state, no use, exiting...");
		return ERROR_WRONG_TARGET_NAME;
	}

	_stprintf_s(szFileName, MAX_PATH, _T("\\\\.\\GLOBALROOT\\Device\\%s"), DRIVERNAME);
	msg(M_DEBUG, L"Attempting to open InpOutNG driver (%s)...", szFileName);

	drvHandle = CreateFile(szFileName, 
		GENERIC_READ | GENERIC_WRITE, 
		0, 
		NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	if(isNotHandle(drvHandle))
	{
		msg((M_DEBUG | M_ERRNO), L"Starting installation of %s driver...", DRIVERNAME);
		
		*pdrvState = DS_INSTALLING;
		
		status = drvInst(pdrvState);

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
/**/
		if (!removeTmpDir())
		{
			msg(M_ERR | M_ERRNO, L"%s::%d, Error occurred while removing temporary directory!", TEXT(__FUNCTION__), __LINE__);
		}
		else
		{
			msg(M_DEBUG, L"%s::%d, Driver was successfully installed. Enjoy!", TEXT(__FUNCTION__), __LINE__);
		}
/**/		
		return ERROR_FILE_NOT_FOUND;
	}

	msg(M_DEBUG, L"Successfully opened %s driver.", DRIVERNAME);
	return 0;
}

BOOL _stdcall IsInpOutDriverOpen()
{
	if (drvState == DS_STARTED)
	{
		drvOpen(bx64, &drvState);
	}
	return ( (drvHandle != INVALID_HANDLE_VALUE) && (drvHandle != NULL) ) ? TRUE : FALSE;
}
