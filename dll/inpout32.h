#pragma once

EXTERN_C_START

#pragma pack(push)
#pragma pack(1)
/**
 * @brief 
*/
typedef struct S_PortTask
{
    USHORT taskOperation;
    USHORT taskPort;
    ULONG  taskData;
} port_task_t, * p_port_task_t;

/**
 * @brief 
*/
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

/**
 * @brief 
*/
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

/**
 * @brief 
*/
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

/**
 * @brief 
 * @param portAddr 
 * @return 
*/
UCHAR   _stdcall inp8( _In_ USHORT portAddr);

/**
 * @brief 
 * @param portAddr 
 * @param portData 
 * @return 
*/
void    _stdcall outp8(_In_ USHORT portAddr, _In_ UCHAR  portData);

/**
 * @brief 
 * @param portAddr 
 * @return 
*/
USHORT  _stdcall inp16( _In_ USHORT portAddr);

/**
 * @brief 
 * @param portAddr 
 * @param portData 
 * @return 
*/
void    _stdcall outp16(_In_ USHORT portAddr, _In_ USHORT portData);

/**
 * @brief 
 * @param portAddr 
 * @return 
*/
ULONG	_stdcall inp32( _In_ USHORT  portAddr);

/**
 * @brief 
 * @param portAddr 
 * @param portData 
 * @return 
*/
void	_stdcall outp32(_In_ USHORT  portAddr, _In_ ULONG  portData);

//My extra functions for making life easy

/**
 * @brief 
 * @param  
 * @return TRUE if the InpOut driver was opened successfully
*/
BOOL	_stdcall IsInpOutDriverOpen(void);

/**
 * @brief 
 * @param  
 * @return TRUE if the OS is 64bit (x64) Windows.
*/
BOOL	_stdcall IsXP64Bit( void );

/**
 * @brief 
*/
extern UINT WM_INPOUTNG_NOTIFY;

/**
 * @brief 
 * @param  
 * @return 
*/
UINT	_stdcall registerWmNotify(void);

/**
 * @brief 
 * @param  
 * @return 
*/
UINT    _stdcall getWmNotify( void );

/**
 * @brief 
 * @param inTask 
 * @param inTaskSize 
 * @param outTask 
 * @param outTaskSize 
 * @return 
*/
BOOL    _stdcall waitForIrq(_In_    p_port_task_t inTask,
                            _In_    DWORD inTaskSize,
                            _Inout_ p_port_task_t outTask,
                            _In_    DWORD outTaskSize);

/**
 * @brief 
 * @param inTask 
 * @param inTaskSize 
 * @param outTask 
 * @param outTaskSize 
 * @param notifyStatus 
 * @return 
*/
VOID    _stdcall requestIrqNotify(_In_      p_port_task_t   inTask,
                                  _In_      DWORD           inTaskSize,
                                  _Inout_   p_port_task_t   outTask,
                                  _In_      DWORD           outTaskSize,
                                  _Out_	    LPDWORD			notifyStatus);

/**
 * @brief 
 * @param  
 * @return 
*/
VOID	_stdcall forceNotify(void);

EXTERN_C_END
