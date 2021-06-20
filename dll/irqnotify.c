#include "StdAfx.h"
#include "datadll.h"
#include <WinUser.h>

UINT WM_INPOUTNG_NOTIFY = 0x0;

typedef struct S_THREAD_PARAMS {
	p_port_task_t	inTask;
	DWORD			inTaskSize;
	p_port_task_t	outTask;
	DWORD			outTaskSize;
	HANDLE*			threadHandle;
} thread_params_t, *p_thread_params_t;

static HANDLE			hNotify =		INVALID_HANDLE_VALUE;
static DWORD			dwNotifyId =	0x0;
static DWORD			dwNotifyStat =  0x0;
static thread_params_t	notifyParams;

static port_task_t sendNotifyTask = { .taskOperation = 0x0, .taskPort = 0x0, .taskData = 0x0 };
static port_task_t recvNotifyTask = { .taskOperation = 0x0, .taskPort = 0x0, .taskData = 0x0 };

UINT    _stdcall getWmNotify( void )
{
	msg(M_DEBUG, L"%s: Returning 0x%04x Message code", TEXT(__FUNCTION__), WM_INPOUTNG_NOTIFY);
	return WM_INPOUTNG_NOTIFY;
}

UINT	_stdcall registerWmNotify(void)
{
	WM_INPOUTNG_NOTIFY=RegisterWindowMessage(L"InpOutNg IRQ Notify");
	msg(M_DEBUG, L"%s: Got 0x%04x Message code", TEXT(__FUNCTION__), WM_INPOUTNG_NOTIFY);
	return getWmNotify();

}

DWORD inpOutNgSendTask(_In_      p_port_task_t inTask,
                       _In_      DWORD inTaskSize,
                       _Inout_   p_port_task_t outTask,
                       _In_      DWORD outTaskSize,
					   _Out_	 LPDWORD szReturned)
{
	DWORD errCode;
	msg(M_DEBUG, L"%s: Calling DeviceIoControl for Irq tasks...", TEXT(__FUNCTION__));
	DeviceIoControl(drvHandle,
		(DWORD)IOCTL_REGISTER_IRQ,
		inTask,
		inTaskSize,
		outTask,
		outTaskSize,
		szReturned,
		NULL);
	errCode = GetLastError();
	msg(M_DEBUG | M_ERRNO, L"%s: DeviceIoControl for Irq tasks finished, status is 0x%x, opCode is %d...", TEXT(__FUNCTION__), errCode, *szReturned);
	return errCode;
}

VOID irqWaitThread(	p_thread_params_t pParams )
{
	BOOL  opSuccess =  FALSE;
	DWORD opCode =	   ERROR_SUCCESS;
	DWORD szReturned = 0x0;
	msg(M_DEBUG, L"%s: Starting irqWaitThread and calling DeviceIoControl...", TEXT(__FUNCTION__));
	opCode = inpOutNgSendTask(
		pParams->inTask,
		pParams->inTaskSize,
		pParams->outTask,
		pParams->outTaskSize,
		&szReturned);
	msg(M_DEBUG | M_ERRNO, L"%s: Returned from DeviceIoControl, status 0x%x, opCode %d...", TEXT(__FUNCTION__), GetLastError(), szReturned);
	opSuccess = (opCode == ERROR_SUCCESS);
	SendMessage(HWND_BROADCAST, WM_INPOUTNG_NOTIFY, opCode, szReturned);
	msg(M_DEBUG, L"%s: Everybody is notified in case of this event (0x%04x)", TEXT(__FUNCTION__), WM_INPOUTNG_NOTIFY);
	pParams->threadHandle = INVALID_HANDLE_VALUE;
}

BOOL    _stdcall waitForIrq(_In_      p_port_task_t inTask,
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

VOID    _stdcall requestIrqNotify(_In_    p_port_task_t	inTask,
								  _In_    DWORD			inTaskSize,
								  _Inout_ p_port_task_t	outTask,
								  _In_    DWORD			outTaskSize,
								  _Out_   LPDWORD		notifyStatus)
{
	if (WM_INPOUTNG_NOTIFY == 0x0)
	{
		registerWmNotify();
	}

	if (isNotHandle(hNotify)) {
		msg(M_DEBUG, L"%s: Creating irqWaitThread...", TEXT(__FUNCTION__));
		notifyParams.inTask		  = inTask;
		notifyParams.inTaskSize   = inTaskSize;
		notifyParams.outTask	  = outTask;
		notifyParams.outTaskSize  = outTaskSize;
		notifyParams.threadHandle = &hNotify;
		hNotify = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)irqWaitThread, &notifyParams, 0, &dwNotifyId);
		msg(M_DEBUG, L"%s: irqWaitThread created, handle is 0x%x...", TEXT(__FUNCTION__), hNotify);
		*notifyStatus = STILL_ACTIVE;
	}
	GetExitCodeThread(hNotify, notifyStatus);
	msg(M_DEBUG, L"%s: Bailing out... handle is 0x%x, status is 0x%x...", TEXT(__FUNCTION__), hNotify, notifyStatus);
}

VOID	_stdcall forceNotify(void)
{
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
}