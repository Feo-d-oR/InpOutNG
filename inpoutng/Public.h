/*++

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.

Environment:

    user and kernel

--*/

//
// Define an Interface Guid so that apps can find the device and talk to it.
//

DEFINE_GUID (GUID_DEVINTERFACE_inpoutng,
    0xf3c34686,0xe4e8,0x43db,0xb3,0x8a,0xad,0xef,0xc6,0xda,0x58,0x51);
// {f3c34686-e4e8-43db-b38a-adefc6da5851}

#define IOCTL_READ_PORT_UCHAR	 -1673519100 //CTL_CODE(40000, 0x801, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_PORT_UCHAR	 -1673519096 //CTL_CODE(40000, 0x802, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_PORT_USHORT	 -1673519092 //CTL_CODE(40000, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_PORT_USHORT	 -1673519088 //CTL_CODE(40000, 0x804, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_READ_PORT_ULONG	 -1673519084 //CTL_CODE(40000, 0x805, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_WRITE_PORT_ULONG	 -1673519080 //CTL_CODE(40000, 0x806, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_WINIO_MAPPHYSTOLIN  -1673519076
#define IOCTL_WINIO_UNMAPPHYSADDR -1673519072

#pragma pack(push)
#pragma pack(1)

struct tagPhys32Struct
{
    HANDLE PhysicalMemoryHandle;
    SIZE_T dwPhysMemSizeInBytes;
    PVOID pvPhysAddress;
    PVOID pvPhysMemLin;
};

#pragma pack(pop)
