# InpOutNG
Next-generation of inpout32 driver &amp; library package

InpOutNG is an open source windows DLL and Driver to give direct access to hardware ports (for example the parallel and serial port from user level programs.
It is originally developed by the people at Logix4U for Windows 9x and 32bit variations of NT (NT/2000/XP/2003 etc.).
Next, the work was continued by [Philip Gibbons](https://www.highrez.co.uk/) and is well-known as [inpout32](https://www.highrez.co.uk/downloads/inpout32/default.htm)
Since 2021 the project development continues here and some new features are introduced.

### Requirements
- Windows 32 or 64-bit (tested on Windows 7 and 10)
- Testing mode or disabled driver signature testing due to Microsoft security policies
- Administrator rights, required only for driver installation procedure
### Features
- Access to ISA/PCI hardware I/O ports
- Task processing - multiple I/O requests can be done in batch mode with a single function call
- IRQ handling - you can assign a hardware IRQ to driver like it was originally made in Windows 95/98 and tell a DLL to notify you about IRQ occurrence or make some task in case of it additionally

## DONATIONS

If you would like to increase my motivation for further development, you can make a donation. 
The amount is not important at all, it is just a sign for me, that time I spent for this project helps someone. 

[DONATE](https://yoomoney.ru/to/4100117182985841)
