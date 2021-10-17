#include "StdAfx.h"
#include "datadll.h"
#include <winerror.h>   //! For error codes

typedef struct _ASYNC_TASK {
    OVERLAPPED  Overlapped;
    port_task_t taskToDo[33];
    port_task_t taskDone[33];
} async_task_t, *p_async_task_t;

static HANDLE           hNotify             = INVALID_HANDLE_VALUE;
static DWORD            dwNotifyId          = 0x0;
static DWORD            dwNotifyStat        = 0x0;
static volatile BOOL    bIrqAck             = FALSE;

static volatile async_task_t asyncTask;

DWORD WINAPI CompletionPortThread(LPVOID PortHandle);

DWORD inpOutNgSendTask(_In_      p_port_task_t inTask,
                       _In_      DWORD inTaskSize,
                       _Inout_   p_port_task_t outTask,
                       _In_      DWORD outTaskSize,
                       _Out_     LPDWORD szReturned)
{
    DWORD errCode;
    DWORD gotIrq;

    msg(M_DEBUG, L"%s: Calling DeviceIoControl for Irq tasks...", TEXT(__FUNCTION__));
    memset((LPVOID)&asyncTask, 0x0, sizeof(OVERLAPPED) + sizeof(port_task_t));
    bIrqAck = FALSE;
    DeviceIoControl(drvHandle,
        (DWORD)IOCTL_REGISTER_IRQ,
        inTask,
        inTaskSize,
        outTask,
        outTaskSize,
        szReturned,
        (LPOVERLAPPED)&asyncTask);
    errCode = GetLastError();
    if (!((errCode != ERROR_IO_PENDING) || (errCode != ERROR_SUCCESS))) {
        msg(M_DEBUG | M_ERRNO, L"%s: DeviceIoControl for Irq tasks finished, status is 0x%x, retCode is %d...", TEXT(__FUNCTION__), errCode, *szReturned);
    }
    while (bIrqAck == FALSE) {
       continue;
    }
    gotIrq = (asyncTask.taskDone[0].taskOperation == INPOUT_IRQ_OCCURRED) ? ERROR_SUCCESS : errCode;
    msg(M_DEBUG | M_ERRNO, L"%s: Irq tasks finished, status is 0x%x, gotIrq is %d...", TEXT(__FUNCTION__), errCode, gotIrq);
    bIrqAck = FALSE;
    return (gotIrq);
}

BOOL WINAPI waitForIrq(VOID)
{
    DWORD ioCode = ERROR_SUCCESS;
    DWORD szReturned = 0x0;
    asyncTask.taskToDo[0].taskOperation = 0x0;
    asyncTask.taskToDo[0].taskPort = 0x11;
    asyncTask.taskToDo[0].taskData = 0x22;
    ioCode = inpOutNgSendTask(
        (p_port_task_t)&asyncTask.taskToDo[0],
        sizeof(port_task_t),
        (p_port_task_t)&asyncTask.taskDone[0],
        sizeof(port_task_t),
        &szReturned);
    return ( (ioCode == ERROR_SUCCESS) ? TRUE : FALSE);
}

BOOL    WINAPI doOnIrq(_In_        p_port_task_t inTask,
                         _In_      DWORD inTaskSize,
                         _Inout_   p_port_task_t outTask,
                         _In_      DWORD outTaskSize)
{
    DWORD ioCode     = ERROR_SUCCESS;
    DWORD opCode     = ERROR_SUCCESS;
    DWORD szReturned = 0x0;
    DWORD inTaskLen  = 0x0;
    DWORD inTaskMax  = 0x0;
    DWORD outTaskLen = 0x0;

    inTaskMax = (sizeof(asyncTask.taskToDo) / sizeof(port_task_t))-1;
    inTaskLen = inTaskSize / sizeof(port_task_t);
    inTaskLen = (inTaskLen > inTaskMax) ? inTaskMax : inTaskLen;
    outTaskLen = outTaskSize / sizeof(port_task_t);

    opCode = memcpy_s(
        (p_port_task_t)&asyncTask.taskToDo[1],
        inTaskMax * sizeof(port_task_t),
        inTask,
        (inTaskLen * sizeof(port_task_t)) );

    ioCode = inpOutNgSendTask(
        (p_port_task_t)&asyncTask.taskToDo,
        inTaskSize + sizeof(port_task_t),
        (p_port_task_t)&asyncTask.taskDone,
        outTaskSize + sizeof(port_task_t),
        &szReturned);

    opCode = memcpy_s(
        outTask,
        outTaskSize,
        (p_port_task_t)&asyncTask.taskDone[1],
        outTaskLen * sizeof(port_task_t));

    return ( ( (ioCode == ERROR_SUCCESS) || (ioCode == ERROR_IO_PENDING))
            && (opCode == ERROR_SUCCESS)) ? TRUE : FALSE;
}

VOID    WINAPI forceNotify(VOID)
{
    port_task_t sendNotifyTask = { .taskOperation = 0x0, .taskPort = 0x0, .taskData = 0x0 };
    port_task_t recvNotifyTask = { .taskOperation = 0x0, .taskPort = 0x0, .taskData = 0x0 };
    DWORD szReturned = 0x0;

    msg(M_DEBUG, L"%s: Forcing driver to make a notification...", TEXT(__FUNCTION__));
    DeviceIoControl(drvHandle,
        (DWORD)IOCTL_TEST_ASYNC,
        &sendNotifyTask,
        sizeof(port_task_t),
        &recvNotifyTask,
        sizeof(port_task_t),
        &szReturned,
        NULL);
    msg(M_DEBUG, L"%s: Exiting...", TEXT(__FUNCTION__));
    
    return;
}

DWORD
WINAPI CompletionPortThread(LPVOID PortHandle)
{
    DWORD byteCount = 0;
    ULONG_PTR compKey = 0;
    OVERLAPPED* overlapped = NULL;
    p_port_task_t ack;
    DWORD code;
    DWORD IrqCountPrev = 0x0;
    volatile DWORD numWait = 0;
    while (TRUE) {

        // Wait for a completion notification.
        overlapped = NULL;

        BOOL worked = GetQueuedCompletionStatus(PortHandle, // Completion port handle
            &byteCount,                                     // Bytes transferred
            &compKey,                                       // Completion key... don't care
            &overlapped,                                    // OVERLAPPED structure
            INFINITE);                                      // Notification time-out interval

        //
        // If it's our notification ioctl that's just been completed...
        // don't do anything special.
        // 
        if (worked && byteCount == 0) {
            continue;
        }

        if (overlapped == NULL) {

            // An unrecoverable error occurred in the completion port.
            // Wait for the next notification.
            continue;
        }

        //
        // Because the wrapper structure STARTS with the OVERLAPPED structure,
        // the pointers are the same.  It would be nicer to use
        // CONTAINING_RECORD here... however you do that in user-mode.
        //

        ack = (p_port_task_t)&asyncTask.taskDone[0];

        code = GetLastError();
        msg(M_DEBUG, L"%s: Got to hard decision, Irq=%d / %d...", TEXT(__FUNCTION__), IrqCountPrev, ack->taskData);
        while ( (bIrqAck == TRUE) && (numWait < 0x2000) ) {
            numWait++;
        }
        numWait = 0;
        if ( (ack->taskOperation == INPOUT_IRQ_OCCURRED)
            &&
            (ack->taskData != IrqCountPrev)
        )
        {
            IrqCountPrev = ack->taskData;
            msg(M_DEBUG, L"%s: Finally it happened!", TEXT(__FUNCTION__), IrqCountPrev);
            bIrqAck = 1;
        }
    }
}