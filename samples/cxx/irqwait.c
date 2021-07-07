// InpoutTest.cpp : Defines the entry point for the console application.
//

#if 0

#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "inpout32.h"

#define ARRAY_SIZE(x) ( sizeof (x) / sizeof(x[0]) )

typedef BOOL (_stdcall* lpWaitForIrq)(_In_    p_port_task_t inTask,
									  _In_    DWORD inTaskSize,
									  _Inout_ p_port_task_t outTask,
									  _In_    DWORD outTaskSize);

typedef BOOL(__stdcall* lpIsInpOutDriverOpen)(void);

//Some global function pointers (messy but fine for an example)
lpIsInpOutDriverOpen gfpIsInpOutDriverOpen;
lpWaitForIrq gfpWaitForIrq;

static port_task_t	taskToDriver[16];
static port_task_t	taskFromDriver[16];

static HANDLE		hNotify = INVALID_HANDLE_VALUE;
static DWORD		dwNotifyId;

VOID irqWaitThread( VOID )
{
	BOOL  opSuccess = FALSE;
	int	  i = 0;
	opSuccess = gfpWaitForIrq(&taskToDriver,   sizeof(taskToDriver),
							  &taskFromDriver, sizeof(taskFromDriver));
	printf(L"%s: Returned from Irq wait!\n", TEXT(__FUNCTION__));
	for (i = 0; i < ARRAY_SIZE(taskFromDriver); i++)
	{
		printf("Task [%d] : code==0x%04x, port==0x%04x, data==0x%08x\n",
			i, taskFromDriver[i].taskOperation,
			taskFromDriver[i].taskPort, taskFromDriver[i].taskData);
	}
}

int main(int argc, char* argv[])
{
	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);
	//Dynamically load the DLL at runtime (not linked at compile time)
	HINSTANCE hInpOutDll;
	DWORD	  dwNotifyStat = 0x0;

	hInpOutDll = LoadLibrary("InpOut32.DLL");

	int numWait = 0;
	int i;
	if (hInpOutDll != NULL)
	{
		gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll, "IsInpOutDriverOpen");
		gfpWaitForIrq = (lpWaitForIrq)GetProcAddress(hInpOutDll, "waitForIrq");
		if (gfpIsInpOutDriverOpen())
		{
			hNotify = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)irqWaitThread, NULL, 0, &dwNotifyId);
			GetExitCodeThread(hNotify, &dwNotifyStat);
			printf(L"%s: irqWaitThread created, handle is 0x%x...", TEXT(__FUNCTION__), hNotify);
			for (i = 0; i < ARRAY_SIZE(taskToDriver), i++)
			{
				taskToDriver[i].taskOperation = (WORD)INPOUT_READ16;
				taskToDriver[i].taskPort = rand(MAXSHORT);
				printf("prepared task [%d] : code==0x%04x, port==0x%04x, data==0x%08x\n",
					i, taskToDriver[i].taskOperation,
					taskToDriver[i].taskPort, taskToDriver[i].taskData);
			}
			while ((numWait++ < 1000) || dwNotifyStat!=STATUS_WAIT_0) {
				Sleep(20);
				GetExitCodeThread(hNotify, &dwNotifyStat);
			}
			if (dwNotifyStat == STATUS_WAIT_0)
			{
				printf("Thread exited successfully, all done!\n");
			} else {
				printf("Thread timeout happened, status is 0x%x!\n", dwNotifyStat);
			}
		}
		else
		{
			printf("Unable to open InpOut32 Driver!\n");
		}
		//All done
		FreeLibrary(hInpOutDll);
		return 0;
	}
	else
	{
		printf("Unable to load InpOut32 DLL!\n");
		return -1;
	}
}
#endif
