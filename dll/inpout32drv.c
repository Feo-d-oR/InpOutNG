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

TCHAR inpPath[MAX_PATH];

#define MYMENU_EXIT         (WM_APP + 101)
#define MYMENU_MESSAGEBOX   (WM_APP + 102)

LRESULT CALLBACK DLLWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (wParam)
		{
		case MYMENU_EXIT:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case MYMENU_MESSAGEBOX:
			MessageBox(hwnd, L"Test", L"MessageBox", MB_OK);
			break;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}

BOOL RegisterDLLWindowClass(wchar_t szClassName[])
{
	WNDCLASSEX wc;
	wc.hInstance = dllInstance;
	wc.lpszClassName = (LPCWSTR)szClassName;
	wc.lpfnWndProc = DLLWindowProc;
	wc.style = CS_DBLCLKS;
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.lpszMenuName = NULL;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	return RegisterClassEx(&wc);
}

HMENU CreateDLLWindowMenu()
{
	HMENU hMenu;
	hMenu = CreateMenu();
	HMENU hMenuPopup;
	if (hMenu == NULL)
		return FALSE;
	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_EXIT, TEXT("Exit"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("File"));

	hMenuPopup = CreatePopupMenu();
	AppendMenu(hMenuPopup, MF_STRING, MYMENU_MESSAGEBOX, TEXT("MessageBox"));
	AppendMenu(hMenu, MF_POPUP, (UINT_PTR)hMenuPopup, TEXT("Test"));
	return hMenu;
}

DWORD WINAPI ThreadProc(LPVOID lpParam)
{
	MSG messages;
	wchar_t* pString = (PWCHAR)lpParam;
	HMENU hMenu = CreateDLLWindowMenu();
	RegisterDLLWindowClass(L"InjectedDLLWindowClass");
	parentHwnd = FindWindow(L"Window Injected Into ClassName", L"Window Injected Into Caption");
	HWND hwnd = CreateWindowEx(0, L"InjectedDLLWindowClass", pString, WS_EX_PALETTEWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300, parentHwnd, hMenu, dllInstance, NULL);
	ShowWindow(hwnd, SW_SHOWNORMAL);
	while (GetMessage(&messages, NULL, 0, 0))
	{
		TranslateMessage(&messages);
		DispatchMessage(&messages);
	}
	return 1;
}

BOOL APIENTRY DllMain(HINSTANCE	hinstDll, 
					  DWORD		fdwReason, 
					  LPVOID	lpReserved)
{
	UNREFERENCED_PARAMETER(lpReserved);
	
	BOOL bx64 = IsXP64Bit();
	
	switch(fdwReason)
	{
		case DLL_PROCESS_ATTACH:
		{
			dllInstance = hinstDll;
			CreateThread(NULL, 0, ThreadProc, (LPVOID)L"Window Title", 0x0, NULL);
			//drvOpen(bx64);
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
