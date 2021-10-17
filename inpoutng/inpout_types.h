/*++

Module Name:

    inpout_types.h

Abstract:

    This file contains the type definitions.

Environment:

    Kernel-mode Driver Framework

--*/
#pragma once

EXTERN_C_START

#define INPOUTNG_VERSION(a,b,c) (((a) << 16) + ((b) << 8) + (c))

#ifdef _NTDDK_
#pragma pack(push)
#pragma pack(1)

/**
 * @brief “ипы задани€ дл€ обработки в драйвере и подтверждени€ выполнени€
*/
typedef enum E_PORT_TASK
{
    INPOUT_IDLE         = 0x00, /** 8'b0000_0000 Ч Ќичего не делать                                 */
    INPOUT_READ8        = 0x01, /** 8'b0000_0001 Ч ¬ыполнить чтение одного байта                    */
    INPOUT_READ16       = 0x02, /** 8'b0000_0010 Ч ¬ыполнить чтение двух байт (слово)               */
    INPOUT_READ32       = 0x04, /** 8'b0000_0100 Ч ¬ыполнить чтение четырЄх байт (двойное слово)    */
    INPOUT_WRITE8       = 0x08, /** 8'b0000_1000 Ч ¬ыполнить запись одного байта                    */
    INPOUT_WRITE16      = 0x10, /** 8'b0001_0000 Ч ¬ыполнить запись двух байт (слово)               */
    INPOUT_WRITE32      = 0x20, /** 8'b0010_0000 Ч ¬ыполнить запись четырЄх байт (двойное слово)    */
    INPOUT_IRQ_OCCURRED = 0x40, /** 8'b0100_0000 Ч ”ведомление о возникновении прерывани€           */
    INPOUT_ACK          = 0x80, /** 8'b1000_0000 Ч ѕодтверждение, резерв                            */
    INPOUT_READ8_ACK    = 0x81, /** 8'b1000_0001 Ч ѕодтверждение выполненного чтени€ байта          */
    INPOUT_READ16_ACK   = 0x82, /** 8'b1000_0010 Ч ѕодтверждение выполненного чтени€ слова          */
    INPOUT_READ32_ACK   = 0x84, /** 8'b1000_0100 Ч ѕодтверждение выполненного чтени€ двойного слова */
    INPOUT_WRITE8_ACK   = 0x88, /** 8'b1000_1000 Ч ѕодтверждение выполненной записи байта           */
    INPOUT_WRITE16_ACK  = 0x90, /** 8'b1001_0000 Ч ѕодтверждение выполненной записи слова           */
    INPOUT_WRITE32_ACK  = 0xa0, /** 8'b1010_0000 Ч ѕодтверждение выполненной записи двойного слова  */
    INPOUT_IRQ_SERVED   = 0xc0  /** 8'b1100_0000 Ч ѕодтверждение, резерв                            */
} port_operation_t, * p_port_operation_t;

typedef struct S_PortTask
{
    USHORT taskOperation;
    USHORT taskPort;
    ULONG  taskData;
} port_task_t, * p_port_task_t;

typedef struct S_InPortData
{
    USHORT addr;
    union U_inPortVal
    {
        UCHAR  inChar;
        USHORT inShrt;
        ULONG  inLong;
    } val;
} inPortData_t, * p_inPortData_t;

typedef struct S_OutPortData
{
    union U_outPortVal
    {
        UCHAR  outChar;
        USHORT outShrt;
        ULONG  outLong;
    } val;
} outPortData_t, * p_outPortData_t;

#pragma pack()
#endif

EXTERN_C_END