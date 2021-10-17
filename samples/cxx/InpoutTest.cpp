// InpoutTest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "windows.h"
#include "stdio.h"

typedef VOID	(WINAPI *lpOut16)(USHORT, USHORT);
typedef USHORT	(WINAPI *lpInp16)(USHORT);
typedef BOOL	(WINAPI *lpIsInpOutDriverOpen)(VOID);
typedef BOOL	(WINAPI *lpIsXP64Bit)(VOID);

//Some global function pointers (messy but fine for an example)
lpOut16 gfpOut16;
lpInp16 gfpInp16;
lpIsInpOutDriverOpen gfpIsInpOutDriverOpen;
lpIsXP64Bit gfpIsXP64Bit;

const USHORT beeper = USHORT(0x42);
const USHORT ctlPit = USHORT(0x43);
const USHORT ctlKbd = USHORT(0x61);

VOID Beep(USHORT freq)
{
	gfpOut16(ctlPit, USHORT(0xB6));
	gfpOut16(beeper, USHORT(freq & USHORT(0xFF)));
	gfpOut16(beeper, USHORT(freq >> 9));
	Sleep(10);
	gfpOut16(ctlKbd, gfpInp16(ctlKbd) | USHORT(0x03));
}

VOID StopBeep()
{
	gfpOut16(ctlKbd, (gfpInp16(ctlKbd) & USHORT(0xFC)));
}

int main(int argc, char* argv[])
{
	if(argc<3)
	{
		//too few command line arguments, show usage
		printf("Error : too few arguments\n\n***** Usage *****\n\nInpoutTest read <ADDRESS> \nor \nInpoutTest write <ADDRESS> <DATA>\n\n\n\n\n");
	} 
	else
	{
		//Dynamically load the DLL at runtime (not linked at compile time)
		HINSTANCE hInpOutDll ;
		hInpOutDll = LoadLibrary ( "InpOut32.DLL" ) ;	//The 32bit DLL. If we are building x64 C++ 
														//applicaiton then use InpOutx64.dll
		if ( hInpOutDll != NULL )
		{
			gfpOut16 = (lpOut16)GetProcAddress(hInpOutDll, "outp16");
			gfpInp16 = (lpInp16)GetProcAddress(hInpOutDll, "inp16");
			gfpIsInpOutDriverOpen = (lpIsInpOutDriverOpen)GetProcAddress(hInpOutDll, "IsInpOutDriverOpen");
			gfpIsXP64Bit = (lpIsXP64Bit)GetProcAddress(hInpOutDll, "IsXP64Bit");

			if (gfpIsInpOutDriverOpen())
			{
				//Make some noise through the PC Speaker - hey it can do more that a single beep using InpOut32
				Beep(2000);
				Sleep(200);
				Beep(1000);
				Sleep(300);
				Beep(2000);
				Sleep(250);
				StopBeep();

				if(!strcmp(argv[1],"read"))
				{
					SHORT  iPort = (SHORT)atoi(argv[2]);
					USHORT wData = gfpInp16(iPort);	//Read the port
					printf("Data read from address %s is %d \n\n\n\n", argv[2], wData);
				}
				else if(!strcmp(argv[1],"write"))
				{
					if(argc<4)
					{
						printf("Error in arguments supplied");
						printf("\n***** Usage *****\n\nInpoutTest read <ADDRESS> \nor \nInpoutTest write <ADDRESS> <DATA>\n\n\n\n\n");
					}
					else
					{
						SHORT  iPort =  (SHORT)atoi(argv[2]);
						USHORT wData = (USHORT)atoi(argv[3]);
						gfpOut16(iPort, wData);
						printf("data written to %s\n\n\n", argv[2]);
					}
				}
			}
			else
			{
				printf("Unable to open InpOut32 Driver!\n");
			}

			//All done
			FreeLibrary ( hInpOutDll ) ;
			return 0;
		}
		else
		{
			printf("Unable to load InpOut32 DLL!\n");
			return -1;
		}
	}
	return -2;
}
