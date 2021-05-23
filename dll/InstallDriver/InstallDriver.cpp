// InstallDriver.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "InstallDriver.h"
#include <inpout32.h>

// The one and only application object
int APIENTRY _tWinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPTSTR    lpCmdLine,
                     int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);
	UNREFERENCED_PARAMETER(nCmdShow);
	int nRetCode = 0;
	
	inpOutNGCreate(L"Чтение/запись портов ISA/PCI InpOutNG", L"ROOT\\inpoutng");
	
	BOOL bResult = IsInpOutDriverOpen();

	if (IsXP64Bit())
	{
		if (bResult)
			MessageBox(NULL, _T("Successfully installed and opened\n64bit InpOut driver InpOutx64.sys."), _T("InpOut Installation"), 0);
		else
			MessageBox(NULL, _T("Unable to install or open the\n64bit InpOut driver InpOutx64.sys.\n\nPlease try running as Administrator"), _T("InpOut Installation"), 0);
	}
	else
	{
		if (bResult)
			MessageBox(NULL, _T("Successfully installed and opened\n32bit InpOut driver InpOut32.sys."), _T("InpOut Installation"), 0);
		else
			MessageBox(NULL, _T("Unable to install or open the\n32bit InpOut driver InpOut32.sys.\n\nPlease try running as Administrator"), _T("InpOut Installation"), 0);
	}
	return nRetCode;
}
