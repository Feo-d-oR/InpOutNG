#pragma once
#include "StdAfx.h"
#include <winioctl.h>
#include <public.h>
#include "inpout32.h"
#include "outmsg.h"

#define DRIVERNAME L"inpoutng\0"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

EXTERN_C_START
EXTERN_C HANDLE drvHandle;
EXTERN_C HINSTANCE dllInstance;

BOOL _stdcall IsXP64Bit( void );
BOOL DisableWOW64(PVOID* oldValue);
BOOL RevertWOW64(PVOID* oldValue);

DWORD drvInst( void );
DWORD drvStart(LPCTSTR pszDriver);
int drvOpen(BOOL bX64);
void drvClose( void );

BOOL isNotHandle(HANDLE h);

EXTERN_C_END
