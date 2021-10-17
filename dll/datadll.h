#pragma once
#include "StdAfx.h"
#include <winioctl.h>
#include <public.h>
#include "inpout32.h"
#include "outmsg.h"

#define DRIVERNAME L"inpoutng\0"
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

EXTERN_C_START
typedef enum E_DRVSTATE {
	DS_STARTUP,
	DS_STARTED,
	DS_INSTALLING,
	DS_READY
} drvInstState_t, * p_drvInstState_t;

EXTERN_C HANDLE			drvHandle;
EXTERN_C HINSTANCE		dllInstance;
EXTERN_C HANDLE         hCompletionPort;
EXTERN_C HANDLE         hCompletionThread;
EXTERN_C DWORD          dwCompletionThreadId;

BOOL
WINAPI
IsXP64Bit(
	VOID
);

BOOL
DisableWOW64 (
	PVOID* oldValue
);

BOOL
RevertWOW64 (
	PVOID* oldValue
);

DWORD
drvInst (
	p_drvInstState_t pdrvState
);

DWORD
drvStart (
	LPCTSTR pszDriver
);

INT drvOpen(
	BOOL bX64,
	p_drvInstState_t pdrvState
);

VOID
drvClose (
	VOID
);

DWORD
inpOutNGCreate(
	_In_opt_	LPCTSTR szDeviceDescription,
	_In_		LPCTSTR szHwId,
	_In_		LPCTSTR cabPath,
	_Inout_		p_drvInstState_t pdrvState
);

VOID outmsg (
	_In_ const unsigned int flags,
	_In_ const TCHAR* format,
	...
);     /* should be called via msg above */

VOID outmsg_va (
	_In_ const unsigned int flags,
	_In_ const TCHAR* format,
	_In_ va_list arglist
);

TCHAR*
getCabFileName (
	VOID
);

TCHAR*
getCabTmpDir ( 
	VOID
);


BOOL
createTmpDir (
	VOID
);

BOOL
unpackCabinet (
	VOID
);

BOOL
removeTmpDir ( 
	VOID
);

DWORD
drvInst (
	p_drvInstState_t pdrvState
);


BOOL
isNotHandle (
	HANDLE h
);

DWORD
WINAPI
CompletionPortThread ( 
	LPVOID PortHandle
);

EXTERN_C_END
