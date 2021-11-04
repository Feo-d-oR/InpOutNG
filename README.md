# InpOutNG
Next-generation of inpout32 driver &amp; library package

*InpOutNG* is an open source windows DLL and Driver to give direct access to hardware ports (for example the parallel and serial port from user level programs.
It is originally developed by the people at Logix4U for Windows 9x and 32bit variations of NT (NT/2000/XP/2003 etc.).
Next, the work was continued by [Philip Gibbons](https://www.highrez.co.uk/) and is well-known as [inpout32](https://www.highrez.co.uk/downloads/inpout32/default.htm)
Since 2021 the project development continues here and some new features are introduced.

Moreover, nowadays one can easily buy brand new hardware with Intel® Core™ i7 onboard, place it to PICMG 1.0 backplane fulled with 14 ISA slots, install Windows 10 and have fun... or some industrial tasks... Almost 40-years bus marked as obsolete for 30 years from now is still alive...

### Requirements
- Windows 32 or 64-bit (tested on Windows 7 and 10)
- Testing mode or disabled driver signature testing due to Microsoft security policies
- Administrator rights, required only for driver installation procedure

### Features
- Access to ISA/PCI hardware I/O ports
- Task processing - multiple I/O requests can be done in batch mode with a single function call
- IRQ handling - you can assign a hardware IRQ to driver like it was originally made in Windows 95/98 and tell a DLL to notify you about IRQ occurrence or make some task in case of it additionally

## Installation of InpOutNG:

### For end users:

Yes, the new version has the same DLL name - `inpout32.dll`. This is made intentionally to provide compatibility with any software that is using original library from Phil Gibbons.

If an application has been written to make use of `inpout32.dll` then all you need to do is place my version of `inpout32.dll` in the same folder as the 'original'. This would usually be the same folder as the applications EXEcutable file. This version is capable of installing and running on both 32 and 64 based operating systems, but the first time it is run, it must be elevated on Vista and later (run as Administrator).

Included in the binary download is a small `InstallDriver.exe` program to do this for you. It is build with «require Administrator» option.

In a windows device manager you can system device for IntOutNG and set an IRQ that the driver will listen. Then, you application can use `waitForIrq` or `doOnIrq` functions that are similar to `WaitForSingleObject` in use.

### For developers:

The driver files are included, as a resource in the DLL. All you need to do is link to the appropriate DLL in your program and it should work.

**NOTE:** When the DLL loads for the first time, the appropriate driver is installed and used. Console window with text output will appear, this is normal.

**NOTE:** Elevated permissions are required in Vista and later to install the driver.

I recommend using runtime linking to the C interface (manually loading the library) in C++, in much the same as the interop calls from .NET work. However you can also use the .LIB and header files provided to link at compile time if that suits your application better.

### Development:

There are examples and some usage instructions in `samples` folder.

Please bear in mind, that on 64bit operating systems, many applications are still, and even now in 2021 are 32bit.

Some documentation will hopefully appear in the doc folder...

### DONATIONS

If you would like to increase my motivation for further development, you can make a donation. 
The amount is not important at all, it is just a sign for me, that time I spent for this project helps someone.
Also, some food for my bear is needed, it makes it calm and it do not disturb my development hobby.

[DONATE](https://yoomoney.ru/to/4100117182985841)
