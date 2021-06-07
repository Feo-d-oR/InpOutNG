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

BOOL _stdcall IsXP64Bit( void );
BOOL DisableWOW64(PVOID* oldValue);
BOOL RevertWOW64(PVOID* oldValue);

DWORD drvInst( p_drvInstState_t pdrvState );
DWORD drvStart(LPCTSTR pszDriver);
int drvOpen(BOOL bX64, p_drvInstState_t pdrvState);
void drvClose( void );

DWORD inpOutNGCreate(_In_opt_	LPCTSTR szDeviceDescription,
					_In_		LPCTSTR szHwId,
					_In_		LPCTSTR cabPath,
					_Inout_		p_drvInstState_t pdrvState);

void outmsg(_In_ const unsigned int flags, _In_ const TCHAR* format, ...);     /* should be called via msg above */

void outmsg_va(_In_ const unsigned int flags, _In_ const TCHAR* format, _In_ va_list arglist);

TCHAR* getCabFileName(void);
TCHAR* getCabTmpDir(void);
BOOL createTmpDir(void);
BOOL unpackCabinet(void);
BOOL removeTmpDir(void);
DWORD drvInst();

BOOL isNotHandle(HANDLE h);


EXTERN_C_END
