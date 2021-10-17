#pragma once

EXTERN_C_START

#pragma pack(push)
#pragma pack(1)

/**
 * @brief Типы задания для обработки в драйвере и подтверждения выполнения
*/
typedef enum E_PORT_TASK
{
    INPOUT_IDLE         = 0x00, /** 8'b0000_0000 — Ничего не делать                                 */
    INPOUT_READ8        = 0x01, /** 8'b0000_0001 — Выполнить чтение одного байта                    */
    INPOUT_READ16       = 0x02, /** 8'b0000_0010 — Выполнить чтение двух байт (слово)               */
    INPOUT_READ32       = 0x04, /** 8'b0000_0100 — Выполнить чтение четырёх байт (двойное слово)    */
    INPOUT_WRITE8       = 0x08, /** 8'b0000_1000 — Выполнить запись одного байта                    */
    INPOUT_WRITE16      = 0x10, /** 8'b0001_0000 — Выполнить запись двух байт (слово)               */
    INPOUT_WRITE32      = 0x20, /** 8'b0010_0000 — Выполнить запись четырёх байт (двойное слово)    */
    INPOUT_IRQ_OCCURRED = 0x40, /** 8'b0100_0000 — Уведомление о возникновении прерывания           */
    INPOUT_ACK          = 0x80, /** 8'b1000_0000 — Подтверждение, резерв                            */
    INPOUT_READ8_ACK    = 0x81, /** 8'b1000_0001 — Подтверждение выполненного чтения байта          */
    INPOUT_READ16_ACK   = 0x82, /** 8'b1000_0010 — Подтверждение выполненного чтения слова          */
    INPOUT_READ32_ACK   = 0x84, /** 8'b1000_0100 — Подтверждение выполненного чтения двойного слова */
    INPOUT_WRITE8_ACK   = 0x88, /** 8'b1000_1000 — Подтверждение выполненной записи байта           */
    INPOUT_WRITE16_ACK  = 0x90, /** 8'b1001_0000 — Подтверждение выполненной записи слова           */
    INPOUT_WRITE32_ACK  = 0xa0, /** 8'b1010_0000 — Подтверждение выполненной записи двойного слова  */
    INPOUT_IRQ_SERVED   = 0xc0  /** 8'b1100_0000 — Подтверждение, резерв                            */
} port_operation_t, * p_port_operation_t;

/**
 * @brief Структура описания задания для драйвера
*/
typedef struct S_PortTask
{
    USHORT taskOperation;   /** Код операции */
    USHORT taskPort;        /** Номер порта, по которому будет производиться чтение/запись */
    ULONG  taskData;        /** Данные передаваемые в оборудование или считываемые из него */
} port_task_t, * p_port_task_t;



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
UCHAR
WINAPI
inp8(
    _In_ USHORT portAddr
);

/**
 * @brief 
 * @param portAddr 
 * @param portData 
 * @return 
*/
VOID
WINAPI
outp8(
    _In_ USHORT portAddr,
    _In_ UCHAR  portData
);

/**
 * @brief 
 * @param portAddr 
 * @return 
*/
USHORT
WINAPI
inp16(
    _In_ USHORT portAddr
);

/**
 * @brief 
 * @param portAddr 
 * @param portData 
 * @return 
*/
VOID
WINAPI
outp16(
    _In_ USHORT portAddr,
    _In_ USHORT portData
);

/**
 * @brief 
 * @param portAddr 
 * @return 
*/
ULONG
WINAPI
inp32(
    _In_ USHORT  portAddr
);

/**
 * @brief 
 * @param portAddr 
 * @param portData 
 * @return 
*/
VOID
WINAPI
outp32(
    _In_ USHORT  portAddr,
    _In_ ULONG  portData
);

/**
 * @brief 
 * @param  
 * @return TRUE if the InpOut driver was opened successfully
*/
BOOL
WINAPI
IsInpOutDriverOpen(VOID);

/**
 * @brief 
 * @param  
 * @return TRUE if the OS is 64bit (x64) Windows.
*/
BOOL
WINAPI
IsXP64Bit( VOID );

/**
 * @brief 
 * @return 
*/
BOOL
WINAPI
waitForIrq(VOID);

/**
 * @brief
 * @param inTask
 * @param inTaskSize
 * @param outTask
 * @param outTaskSize
 * @return
*/
BOOL
WINAPI
doOnIrq(
    _In_    p_port_task_t inTask,
    _In_    DWORD inTaskSize,
    _Inout_ p_port_task_t outTask,
    _In_    DWORD outTaskSize
);

/**
 * @brief 
 * @param  
 * @return 
*/
VOID
WINAPI
forceNotify(VOID);

/**
 * @brief
 * @param
 * @return
*/
ULONG
WINAPI
irqCount( VOID );

/**
 * @brief
 * @param inTask
 * @param inTaskSize
 * @return
*/
BOOL
WINAPI
setIrqClearSeq(
    _In_    p_port_task_t inTask,
    _In_    DWORD inTaskSize);

EXTERN_C_END
