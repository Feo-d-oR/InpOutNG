#include "StdAfx.h"
#include "datadll.h"
#include <WinUser.h>

UINT WM_INPOUTNG_NOTIFY = 0x0;

typedef struct S_THREAD_PARAMS {
    p_port_task_t    inTask;
    DWORD            inTaskSize;
    p_port_task_t    outTask;
    DWORD            outTaskSize;
    HANDLE*          threadHandle;
} thread_params_t, *p_thread_params_t;

typedef struct _ASYNC_TASK {
    OVERLAPPED  Overlapped;
    port_task_t recvNotifyTask;
} async_task_t, *p_async_task_t;

static HANDLE           hNotify             = INVALID_HANDLE_VALUE;
static DWORD            dwNotifyId          = 0x0;
static DWORD            dwNotifyStat        = 0x0;
static thread_params_t  notifyParams;
static volatile BOOL    bIrqAck             = FALSE;
static port_task_t sendNotifyTask = { .taskOperation = 0x0, .taskPort = 0x0, .taskData = 0x0 };
//static port_task_t recvNotifyTask = { .taskOperation = 0x0, .taskPort = 0x0, .taskData = 0x0 };

static volatile async_task_t asyncTask;

DWORD WINAPI CompletionPortThread(LPVOID PortHandle);

UINT    _stdcall getWmNotify( void )
{
    msg(M_DEBUG, L"%s: Returning 0x%04x Message code", TEXT(__FUNCTION__), WM_INPOUTNG_NOTIFY);
    return WM_INPOUTNG_NOTIFY;
}

UINT    _stdcall registerWmNotify(void)
{
    WM_INPOUTNG_NOTIFY=RegisterWindowMessage(L"InpOutNg IRQ Notify");
    msg(M_DEBUG, L"%s: Got 0x%04x Message code", TEXT(__FUNCTION__), WM_INPOUTNG_NOTIFY);
    return getWmNotify();

}

DWORD inpOutNgSendTask(_In_      p_port_task_t inTask,
                       _In_      DWORD inTaskSize,
                       _Inout_   p_port_task_t outTask,
                       _In_      DWORD outTaskSize,
                       _Out_     LPDWORD szReturned)
{
    DWORD errCode;
    BOOL  gotIrq;
    msg(M_DEBUG, L"%s: Calling DeviceIoControl for Irq tasks...", TEXT(__FUNCTION__));
    memset((LPVOID)&asyncTask, 0x0, sizeof(asyncTask));
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
    msg(M_DEBUG | M_ERRNO, L"%s: DeviceIoControl for Irq tasks finished, status is 0x%x, retCode is %d...", TEXT(__FUNCTION__), errCode, *szReturned);
    while (bIrqAck == FALSE) {
       continue;
    }
    gotIrq = (asyncTask.recvNotifyTask.taskOperation == INPOUT_IRQ_OCCURRED);
    msg(M_DEBUG | M_ERRNO, L"%s: Irq tasks finished, status is 0x%x, gotIrq is %d...", TEXT(__FUNCTION__), errCode, gotIrq);
    bIrqAck = FALSE;
    return (gotIrq);
}

VOID irqWaitThread( p_thread_params_t pParams )
{
    BOOL  opSuccess =  FALSE;
    DWORD opCode =       ERROR_SUCCESS;
    DWORD szReturned = 0x0;
    msg(M_DEBUG, L"%s: Starting irqWaitThread and calling DeviceIoControl...", TEXT(__FUNCTION__));
    bIrqAck = FALSE;
    opCode = inpOutNgSendTask(
        pParams->inTask,
        pParams->inTaskSize,
        pParams->outTask,
        pParams->outTaskSize,
        &szReturned);
    msg(M_DEBUG | M_ERRNO, L"%s: Returned from DeviceIoControl, status 0x%x, opCode %d...", TEXT(__FUNCTION__), GetLastError(), szReturned);
    while (bIrqAck != TRUE) {
        continue;
    }
    bIrqAck = FALSE;
    opSuccess = (asyncTask.recvNotifyTask.taskOperation == INPOUT_IRQ_OCCURRED);
    SendMessage(HWND_BROADCAST, WM_INPOUTNG_NOTIFY, opCode, szReturned);
    msg(M_DEBUG, L"%s: Everybody is notified in case of this event (0x%04x)", TEXT(__FUNCTION__), WM_INPOUTNG_NOTIFY);
    pParams->threadHandle = INVALID_HANDLE_VALUE;
}

BOOL    _stdcall waitForIrq(void)
{
    DWORD opCode = ERROR_SUCCESS;
    DWORD szReturned = 0x0;
    port_task_t inTask = { .taskOperation = 0x0, .taskPort = 0x0, .taskData = 0x0 };
    opCode = inpOutNgSendTask(
        &inTask,
        sizeof(inTask),
        (p_port_task_t)&asyncTask.recvNotifyTask,
        sizeof(port_task_t),
        &szReturned);
    return (opCode == ERROR_SUCCESS);
}

BOOL    _stdcall doOnIrq(_In_      p_port_task_t inTask,
                            _In_      DWORD inTaskSize,
                            _Inout_   p_port_task_t outTask,
                            _In_      DWORD outTaskSize)
{
    DWORD opCode = ERROR_SUCCESS;
    DWORD szReturned = 0x0;

    opCode = inpOutNgSendTask(
        inTask,
        inTaskSize,
        outTask,
        outTaskSize,
        &szReturned);
    return (opCode == ERROR_SUCCESS);
}

VOID    _stdcall requestIrqNotify(_In_    p_port_task_t    inTask,
                                  _In_    DWORD            inTaskSize,
                                  _Inout_ p_port_task_t    outTask,
                                  _In_    DWORD            outTaskSize,
                                  _Out_   LPDWORD        notifyStatus)
{
    if (WM_INPOUTNG_NOTIFY == 0x0)
    {
        registerWmNotify();
    }

    if (isNotHandle(hNotify)) {
        msg(M_DEBUG, L"%s: Creating irqWaitThread...", TEXT(__FUNCTION__));
        notifyParams.inTask       = inTask;
        notifyParams.inTaskSize   = inTaskSize;
        notifyParams.outTask      = outTask;
        notifyParams.outTaskSize  = outTaskSize;
        notifyParams.threadHandle = &hNotify;
        hNotify = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)irqWaitThread, &notifyParams, 0, &dwNotifyId);
        msg(M_DEBUG, L"%s: irqWaitThread created, handle is 0x%x...", TEXT(__FUNCTION__), hNotify);
        *notifyStatus = STILL_ACTIVE;
    }
    GetExitCodeThread(hNotify, notifyStatus);
    msg(M_DEBUG, L"%s: Bailing out... handle is 0x%x, status is 0x%x...", TEXT(__FUNCTION__), hNotify, notifyStatus);
}

VOID    _stdcall forceNotify(void)
{
    DWORD szReturned = 0x0;
    msg(M_DEBUG, L"%s: Forcing driver to make a notification...", TEXT(__FUNCTION__));
    DeviceIoControl(drvHandle,
        (DWORD)IOCTL_TEST_ASYNC,
        &sendNotifyTask,
        sizeof(port_task_t),
        (p_port_task_t)&asyncTask.recvNotifyTask,
        sizeof(port_task_t),
        &szReturned,
        (LPOVERLAPPED)&asyncTask);
    msg(M_DEBUG, L"%s: Exiting...", TEXT(__FUNCTION__));
}

DWORD
WINAPI CompletionPortThread(LPVOID PortHandle)
{
    DWORD byteCount = 0;
    ULONG_PTR compKey = 0;
    OVERLAPPED* overlapped = NULL;
    p_async_task_t wrap;
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
        wrap = (p_async_task_t)(overlapped);

        code = GetLastError();
        msg(M_DEBUG, L"%s: Got to hard decision, Irq=%d / %d...", TEXT(__FUNCTION__), IrqCountPrev, wrap->recvNotifyTask[0].taskData);
        while ( (bIrqAck == TRUE) && (numWait < 0x2000) ) {
            numWait++;
        }
        numWait = 0;
        if ( (wrap->recvNotifyTask.taskOperation == INPOUT_IRQ_OCCURRED)
            &&
            (wrap->recvNotifyTask.taskData != IrqCountPrev)
        )
        {
            IrqCountPrev = wrap->recvNotifyTask.taskData;
            msg(M_DEBUG, L"%s: Finally it happened!", TEXT(__FUNCTION__), IrqCountPrev);
            bIrqAck = 1;
        }
    }
}