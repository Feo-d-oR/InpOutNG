#pragma once
#include "StdAfx.h"
#include <winioctl.h>
#include <public.h>
#include "inpout32.h"
#include "outmsg.h"

EXTERN_C_START
EXTERN_C HANDLE drvHandle;
EXTERN_C HINSTANCE inpInstance;
EXTERN_C SECURITY_ATTRIBUTES inpSa;
EXTERN_C TCHAR inpPath[MAX_PATH];

//First, lets set the DRIVERNAME depending on our configuraiton.
//!!#define DRIVERNAMEx64 _T("inpoutng64\0")
#define DRIVERNAMEx64 _T("inpoutng\0")
#define DRIVERNAMEx86 _T("inpoutng\0")

BOOL _stdcall IsXP64Bit( void );
BOOL DisableWOW64(PVOID* oldValue);
BOOL RevertWOW64(PVOID* oldValue);

int inst32( void );
int inst64( void );
int start(LPCTSTR pszDriver);
int drvOpen(BOOL bX64);
void drvClose( void );
EXTERN_C_END
