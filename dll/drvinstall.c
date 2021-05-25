#include <windows.h>
#include <cfgmgr32.h>
#include <objbase.h>
#include <setupapi.h>
#include <stdio.h>
#include <tchar.h>
#include <newdev.h>

#include "datadll.h"
#include "drvinstall.h"
#include "resource.h"

#ifdef _MSC_VER
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "newdev.lib")
#endif

#ifdef _MSC_VER
#pragma warning(disable: 4152) /* EXIT_FATAL(flags) macro raises "warning C4127: conditional expression is constant" on each non M_FATAL invocation. */
#endif

/**
 * Dynamically load a library and find a function in it
 *
 * @param libname     Name of the library to load
 * @param funcname    Name of the function to find
 * @param m           Pointer to a module. On return this is set to the
 *                    the handle to the loaded library. The caller must
 *                    free it by calling FreeLibrary() if not NULL.
 *
 * @return            Pointer to the function
 *                    NULL on error -- use GetLastError() to find the error code.
 *
 **/
static void*
find_function(const WCHAR* libname, const char* funcname, HMODULE* m)
{
	WCHAR libpath[MAX_PATH];
	void* fptr = NULL;

	/* Make sure the dll is loaded from the system32 folder */
	if (!GetSystemDirectoryW(libpath, ARRAY_SIZE(libpath)))
	{
		return NULL;
	}

	size_t len = ARRAY_SIZE(libpath) - wcslen(libpath) - 1;
	if (len < wcslen(libname) + 1)
	{
		SetLastError(ERROR_INSUFFICIENT_BUFFER);
		return NULL;
	}
	wcsncat_s(libpath, MAX_PATH, L"\\", len);
	wcsncat_s(libpath, MAX_PATH, libname, len - 1);

	*m = LoadLibraryW(libpath);
	if (*m == NULL)
	{
		return NULL;
	}
	fptr = GetProcAddress(*m, funcname);
	if (!fptr)
	{
		FreeLibrary(*m);
		*m = NULL;
		return NULL;
	}
	return fptr;
}

/**
 * Checks device install parameters if a system reboot is required.
 *
 * @param hDeviceInfoSet  A handle to a device information set that contains a device
 *                      information element that represents the device.
 *
 * @param pDeviceInfoData  A pointer to an SP_DEVINFO_DATA structure that specifies the
 *                      device information element in hDeviceInfoSet.
 *
 * @param pbRebootRequired  A pointer to a BOOL flag. If the device requires a system restart,
 *                      this flag is set to TRUE. Otherwise, the flag is left unmodified. This
 *                      allows the flag to be globally initialized to FALSE and reused for multiple
 *                      adapter manipulations.
 *
 * @return ERROR_SUCCESS on success; Win32 error code otherwise
 **/
static DWORD
check_reboot(
	_In_ HDEVINFO hDeviceInfoSet,
	_In_ PSP_DEVINFO_DATA pDeviceInfoData,
	_Inout_ LPBOOL pbRebootRequired)
{
	if (pbRebootRequired == NULL)
	{
		return ERROR_BAD_ARGUMENTS;
	}

	SP_DEVINSTALL_PARAMS devinstall_params = { .cbSize = sizeof(SP_DEVINSTALL_PARAMS) };
	if (!SetupDiGetDeviceInstallParams(
		hDeviceInfoSet,
		pDeviceInfoData,
		&devinstall_params))
	{
		DWORD dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s: SetupDiGetDeviceInstallParams failed", TEXT(__FUNCTION__));
		return dwResult;
	}

	if ((devinstall_params.Flags & (DI_NEEDREBOOT | DI_NEEDRESTART)) != 0)
	{
		*pbRebootRequired = TRUE;
	}

	return ERROR_SUCCESS;
}
static TCHAR drvPath[MAX_PATH];
DWORD
inpOutNGCreate(
	_In_opt_ LPCTSTR szDeviceDescription,
	_In_ LPCTSTR szHwId,
	_In_ LPCTSTR cabPath)
{
	DWORD	dwResult = ERROR_SUCCESS;
	HMODULE libnewdev = NULL;
	DWORD	drvInstFlags = DIIRFLAG_FORCE_INF;
	BOOL	rebootRequired;
	GUID    GUID_DEV_INPOUTNG = { 0xf3c34686, 0xe4e8, 0x43db, 0xb3, 0x8a, 0xad, 0xef, 0xc6, 0xda, 0x58, 0x51 };
	// {f3c34686-e4e8-43db-b38a-adefc6da5851}
	
	ZeroMemory(drvPath, sizeof(drvPath));

	_stprintf_s(drvPath, ARRAY_SIZE(drvPath), L"%s%s", cabPath, IsXP64Bit() ? L"x64" : L"x86");
	
	msg(M_DEBUG, L"%s::%d Trying to install InpOutNG driver from %s", TEXT(__FUNCTION__), __LINE__, drvPath);

	if (!DiInstallDriver(NULL, drvPath, drvInstFlags, &rebootRequired))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d DiInstallDevice failed", TEXT(__FUNCTION__), __LINE__);
		//goto cleanup_remove_device;
	}

	msg(M_DEBUG, L"Device installation finished, last Error code is 0x%x", dwResult);
	//return dwResult;
	if (szHwId == NULL)
	{
		return ERROR_BAD_ARGUMENTS;
	}

	/* Create an empty device info set for network adapter device class. */
	HDEVINFO hDevInfoList = SetupDiCreateDeviceInfoList(&GUID_DEV_INPOUTNG, NULL);
	if (hDevInfoList == INVALID_HANDLE_VALUE)
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiCreateDeviceInfoList failed", TEXT(__FUNCTION__), __LINE__);
		return dwResult;
	}

	/* Get the device class name from GUID. */
	TCHAR szClassName[MAX_CLASS_NAME_LEN];
	if (!SetupDiClassNameFromGuid(
		&GUID_DEVCLASS_SYSTEM,
		szClassName,
		ARRAY_SIZE(szClassName),
		NULL))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL, L"%s::%d SetupDiClassNameFromGuid failed", TEXT(__FUNCTION__), __LINE__);
		goto cleanup_hDevInfoList;
	}

	/* Create a new device info element and add it to the device info set. */
	SP_DEVINFO_DATA devinfo_data = { .cbSize = sizeof(SP_DEVINFO_DATA) };
	if (!SetupDiCreateDeviceInfo(
		hDevInfoList,
		szClassName,
		&GUID_DEV_INPOUTNG,
		szDeviceDescription,
		NULL,
		DICD_GENERATE_ID,
		&devinfo_data))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiCreateDeviceInfo failed", TEXT(__FUNCTION__), __LINE__);
		goto cleanup_hDevInfoList;
	}

	/* Set a device information element as the selected member of a device information set. */
	if (!SetupDiSetSelectedDevice(
		hDevInfoList,
		&devinfo_data))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiSetSelectedDevice failed", TEXT(__FUNCTION__), __LINE__);
		goto cleanup_hDevInfoList;
	}

	/* Set Plug&Play device hardware ID property. */
	if (!SetupDiSetDeviceRegistryProperty(
		hDevInfoList,
		&devinfo_data,
		SPDRP_HARDWAREID,
		(const BYTE*)szHwId, (DWORD)((_tcslen(szHwId) + 1) * sizeof(TCHAR))))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiSetDeviceRegistryProperty failed", TEXT(__FUNCTION__), __LINE__);
		goto cleanup_hDevInfoList;
	}

#if 0
	/* Allow the device instance installation with the PnP Manager */
	if (!SetupDiCallClassInstaller(
		DIF_ALLOW_INSTALL,
		hDevInfoList,
		&devinfo_data))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiCallClassInstaller(DIF_ALLOW_INSTALL) failed", TEXT(__FUNCTION__), __LINE__);
		//goto cleanup_hDevInfoList;
	}

	/* Install device instance files with the PnP Manager */
	if (!SetupDiCallClassInstaller(
		DIF_INSTALLDEVICEFILES,
		hDevInfoList,
		&devinfo_data))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES) failed", TEXT(__FUNCTION__), __LINE__);
		//goto cleanup_hDevInfoList;
	}
#endif

	/* Register device instance Co-installer files with the PnP Manager */
	if (!SetupDiCallClassInstaller(
		DIF_REGISTER_COINSTALLERS,
		hDevInfoList,
		&devinfo_data))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS) failed", TEXT(__FUNCTION__), __LINE__);
		//goto cleanup_hDevInfoList;
	}

	/* Install device instance interfaces with the PnP Manager */
	if (!SetupDiCallClassInstaller(
		DIF_INSTALLINTERFACES,
		hDevInfoList,
		&devinfo_data))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiCallClassInstaller(DIF_INSTALLINTERFACES) failed", TEXT(__FUNCTION__), __LINE__);
		//goto cleanup_hDevInfoList;
	}

#if 1
	/* Install device instance with the PnP Manager */
	if (!SetupDiCallClassInstaller(
		DIF_NEWDEVICEWIZARD_FINISHINSTALL,
		hDevInfoList,
		&devinfo_data))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiCallClassInstaller(DIF_NEWDEVICEWIZARD_FINISHINSTALL) failed", TEXT(__FUNCTION__), __LINE__);
		//goto cleanup_hDevInfoList;
	}
#endif

	/* Register the device instance with the PnP Manager */
	if (!SetupDiCallClassInstaller(
		DIF_REGISTERDEVICE,
		hDevInfoList,
		&devinfo_data))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiCallClassInstaller(DIF_REGISTERDEVICE) failed", TEXT(__FUNCTION__), __LINE__);
		//goto cleanup_hDevInfoList;
	}

	msg(M_DEBUG, L"Setup chain finished, last Error code is 0x%x", dwResult);

	/* Install the device using DiInstallDevice()
	 * We instruct the system to use the best driver in the driver store
	 * by setting the drvinfo argument of DiInstallDevice as NULL. This
	 * assumes a driver is already installed in the driver store.
	 */
	if (!DiInstallDevice(NULL, hDevInfoList, &devinfo_data, NULL, DIIDFLAG_INSTALLCOPYINFDRIVERS, &rebootRequired))
	//if (!DiInstallDriver(NULL, drvPath, drvInstFlags, &rebootRequired))
	{
		dwResult = GetLastError();
		msg(M_NONFATAL | M_ERRNO, L"%s::%d DiInstallDevice failed", TEXT(__FUNCTION__), __LINE__);
		goto cleanup_remove_device;
	}

	msg(M_DEBUG, L"Device installation finished, last Error code is 0x%x", dwResult);

cleanup_remove_device:
#if 0
	if (dwResult != ERROR_SUCCESS)
	{
		/* The adapter was installed. But, the adapter ID was unobtainable. Clean-up. */
		SP_REMOVEDEVICE_PARAMS removedevice_params =
		{
			.ClassInstallHeader =
			{
				.cbSize = sizeof(SP_CLASSINSTALL_HEADER),
				.InstallFunction = DIF_REMOVE,
			},
			.Scope = DI_REMOVEDEVICE_GLOBAL,
			.HwProfile = 0,
		};

		/* Set class installer parameters for DIF_REMOVE. */
		if (SetupDiSetClassInstallParams(
			hDevInfoList,
			&devinfo_data,
			&removedevice_params.ClassInstallHeader,
			sizeof(SP_REMOVEDEVICE_PARAMS)))
		{
			/* Call appropriate class installer. */
			if (SetupDiCallClassInstaller(
				DIF_REMOVE,
				hDevInfoList,
				&devinfo_data))
			{
				/* Check if a system reboot is required. */
				check_reboot(hDevInfoList, &devinfo_data, pbRebootRequired);
			}
			else
			{
				msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiCallClassInstaller(DIF_REMOVE) failed", TEXT(__FUNCTION__), __LINE__);
			}
		}
		else
		{
			msg(M_NONFATAL | M_ERRNO, L"%s::%d SetupDiSetClassInstallParams failed", TEXT(__FUNCTION__), __LINE__);
		}
	}
#endif
cleanup_hDevInfoList:
	if (libnewdev)
	{
		FreeLibrary(libnewdev);
	}
	SetupDiDestroyDeviceInfoList(hDevInfoList);
	return dwResult;
}

/***********************************************************************/
DWORD drvInst()
{
	DWORD result = ERROR_FAILED_DRIVER_ENTRY;
	if (!unpackCabinet())
	{
		msg(M_ERR | M_ERRNO, L"%s::%d, Error occurred while unpack CAB file!", TEXT(__FUNCTION__), __LINE__);
	}
	else
	{
		result = inpOutNGCreate(L"Чтение/запись портов ISA/PCI InpOutNG", L"ROOT\\inpoutng", getCabTmpDir());
		if (result != ERROR_SUCCESS)
		{
			msg(M_ERR | M_ERRNO, L"%s::%d, Error occurred while driver installation routine!", TEXT(__FUNCTION__), __LINE__);
		}
		if (!removeTmpDir())
		{
			msg(M_ERR | M_ERRNO, L"%s::%d, Error occurred while removing temporary directory!", TEXT(__FUNCTION__), __LINE__);
		}
		else
		{
			msg(M_DEBUG, L"%s::%d, Driver was successfully installed. Enjoy!", TEXT(__FUNCTION__), __LINE__);
		}
	}
	return result;
}

/**************************************************************************/
DWORD drvStart(LPCTSTR pszDriver)
{
	SC_HANDLE  Mgr;
	SC_HANDLE  Ser;

	Mgr = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);

	if (Mgr == NULL)
	{							//No permission to create service
		if (GetLastError() == ERROR_ACCESS_DENIED)
		{
			Mgr = OpenSCManager(NULL, NULL, GENERIC_READ);
			Ser = OpenService(Mgr, pszDriver, GENERIC_EXECUTE);
			if (Ser)
			{    // we have permission to start the service
				if (!StartService(Ser, 0, NULL))
				{
					CloseServiceHandle(Ser);
					return ERROR_SERVICE_NOT_ACTIVE; // we could open the service but unable to start
				}
			}
		}
	}
	else
	{// Successfuly opened Service Manager with full access
		Ser = OpenService(Mgr, pszDriver, SERVICE_ALL_ACCESS);
		if (Ser)
		{
			if (!StartService(Ser, 0, NULL))
			{
				CloseServiceHandle(Ser);
				return ERROR_SERVICE_START_HANG; // opened the Service handle with full access permission, but unable to start
			}
			else
			{
				CloseServiceHandle(Ser);
				return ERROR_SUCCESS;
			}
		}
	}
	return 1;
}
