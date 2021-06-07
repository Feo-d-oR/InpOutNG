#pragma once

EXTERN_C_START

// 3 группы функций: inp8/outp8, ...16, ...32, чтобы были однотипные названия.
UCHAR   _stdcall inp8( _In_ USHORT portAddr);
void    _stdcall outp8(_In_ USHORT portAddr, _In_ UCHAR  portData);

USHORT  _stdcall inp16( _In_ USHORT portAddr);
void    _stdcall outp16(_In_ USHORT portAddr, _In_ USHORT portData);

ULONG	_stdcall inp32( _In_ USHORT  portAddr);
void	_stdcall outp32(_In_ USHORT  portAddr, _In_ ULONG  portData);

//My extra functions for making life easy
BOOL	_stdcall IsInpOutDriverOpen( void );	//Returns TRUE if the InpOut driver was opened successfully
BOOL	_stdcall IsXP64Bit( void );				//Returns TRUE if the OS is 64bit (x64) Windows.

EXTERN_C_END
