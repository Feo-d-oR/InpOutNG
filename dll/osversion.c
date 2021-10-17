#include "stdafx.h"

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
typedef BOOL (WINAPI *LPFN_WOW64DISABLE) (PVOID*);
typedef BOOL (WINAPI *LPFN_WOW64REVERT) (PVOID);

//Purpose: Return TRUE if we are running in WOW64 (i.e. a 32bit process on XP x64 edition)
BOOL
WINAPI
IsXP64Bit (
    VOID
)
{
#ifdef _M_X64
    return TRUE;    //Urrr if its a x64 build of the DLL, we MUST be running on X64 nativly!
#else
    LPFN_ISWOW64PROCESS    fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(GetModuleHandle(_T("kernel32")), "IsWow64Process");
    BOOL bIsWow64 = FALSE;
     if (NULL != fnIsWow64Process)
    {
        if (!fnIsWow64Process(GetCurrentProcess(),&bIsWow64))
        {
            // handle error
        }
    }
    return bIsWow64;
#endif
}

BOOL
DisableWOW64 (
    PVOID* oldValue
)
{
#ifdef _M_X64
    UNREFERENCED_PARAMETER(oldValue);
    return TRUE;        // If were 64b under x64, we dont wanna do anything!
#else
    LPFN_WOW64DISABLE    fnWow64Disable = (LPFN_WOW64DISABLE)GetProcAddress(GetModuleHandle(_T("kernel32")), "Wow64DisableWow64FsRedirection");
    return fnWow64Disable(oldValue);
#endif
}

BOOL
RevertWOW64 (
    PVOID* oldValue
)
{
#ifdef _M_X64
    UNREFERENCED_PARAMETER(oldValue);
    return TRUE;        // If were 64b under x64, we dont wanna do anything!
#else
    LPFN_WOW64REVERT    fnWow64Revert = (LPFN_WOW64REVERT)GetProcAddress(GetModuleHandle(_T("kernel32")), "Wow64RevertWow64FsRedirection");
    return fnWow64Revert (oldValue);
#endif
}
