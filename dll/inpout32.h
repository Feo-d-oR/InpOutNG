#pragma once

EXTERN_C_START

//Functions exported from DLL.
//For easy inclusion is user projects.
//Original InpOut32 function support
void	_stdcall Out32(_In_ SHORT portAddr, _In_ SHORT portData);
SHORT	_stdcall Inp32(_In_ SHORT portAddr);

//My extra functions for making life easy
BOOL	_stdcall IsInpOutDriverOpen( void );	//Returns TRUE if the InpOut driver was opened successfully
BOOL	_stdcall IsXP64Bit( void );				//Returns TRUE if the OS is 64bit (x64) Windows.

//DLLPortIO function support
UCHAR   _stdcall DlPortReadPortUchar  (_In_ USHORT portAddr);
USHORT  _stdcall DlPortReadPortUshort (_In_ USHORT portAddr);
ULONG	_stdcall DlPortReadPortUlong  (_In_ ULONG  portAddr);
void    _stdcall DlPortWritePortUchar (_In_ USHORT portAddr, _In_ UCHAR  portData);
void    _stdcall DlPortWritePortUshort(_In_ USHORT portAddr, _In_ USHORT portData);
void	_stdcall DlPortWritePortUlong (_In_ ULONG  portAddr, _In_ ULONG  portData);

//WinIO function support (Untested and probably does NOT work - esp. on x64!)
PBYTE	_stdcall MapPhysToLin(_In_ PBYTE pbPhysAddr, _In_ DWORD dwPhysSize, _Inout_ HANDLE *pPhysicalMemoryHandle);
BOOL	_stdcall UnmapPhysicalMemory(_Inout_ HANDLE PhysicalMemoryHandle, _In_ PBYTE pbLinAddr);
BOOL	_stdcall GetPhysLong(_In_ PBYTE pbPhysAddr, _Out_ PDWORD pdwPhysVal);
BOOL	_stdcall SetPhysLong(_In_ PBYTE pbPhysAddr, _Out_ DWORD dwPhysVal);

DWORD inpOutNGCreate(_In_opt_ LPCTSTR szDeviceDescription, _In_ LPCTSTR szHwId, _Inout_ LPBOOL pbRebootRequired, _Out_ LPGUID pguidAdapter);

void outmsg(_In_ const unsigned int flags, _In_ const TCHAR* format, ...);     /* should be called via msg above */

void outmsg_va(_In_ const unsigned int flags, _In_ const TCHAR* format, _In_ va_list arglist);

EXTERN_C_END
