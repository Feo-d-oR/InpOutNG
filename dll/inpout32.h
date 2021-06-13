#pragma once

EXTERN_C_START

#pragma pack(push)
#pragma pack(1)
typedef struct S_PortTask
{
    USHORT taskOperation;
    USHORT taskPort;
    ULONG  taskData;
} port_task_t, * p_port_task_t;

typedef enum E_PORT_TASK
{
    INPOUT_NOTIFY =         0x00,
    INPOUT_READ8 =          0x01,
    INPOUT_READ16 =         0x02,
    INPOUT_READ32 =         0x04,
    INPOUT_WRITE8 =         0x08,
    INPOUT_WRITE16 =        0x10,
    INPOUT_WRITE32 =        0x20,
    INPOUT_ACK =            0x80,
    INPOUT_READ8_ACK =      0x81,
    INPOUT_READ16_ACK =     0x82,
    INPOUT_READ32_ACK =     0x84,
    INPOUT_WRITE8_ACK =     0x88,
    INPOUT_WRITE16_ACK =    0x90,
    INPOUT_WRITE32_ACK =    0xa0
} port_operation_t, * p_port_operation_t;

typedef struct S_OutPortData
{
    USHORT addr;
    union U_outPortVal
    {
        UCHAR  outChar;
        USHORT outShrt;
        ULONG  outLong;
    } val;
} outPortData_t, * p_outPortData_t;

typedef struct S_InPortData
{
    union U_inPortVal
    {
        UCHAR  inChar;
        USHORT inShrt;
        ULONG  inLong;
    } val;
} inPortData_t, * p_inPortData_t;
#pragma pack()

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
