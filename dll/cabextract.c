#include "datadll.h"
#include "resource.h"
#include <shellapi.h>


static errno_t status;
static TCHAR tmpDir[MAX_PATH] = { 0x0 };
static TCHAR fileName[MAX_PATH] = { 0x0 };
static TCHAR expandParams[MAX_PATH] = { 0x0 };
static TCHAR expandFile[MAX_PATH] = { 0x0 };
static TCHAR expandDir[MAX_PATH] = { 0x0 };
TCHAR* getCabFileName( void )
{
	return fileName;
}

TCHAR* getCabTmpDir( void )
{
	return tmpDir;
}

BOOL createTmpDir( void )
{
	BOOL result = FALSE;
	TCHAR dirName[MAX_PATH];
	memset(tmpDir, 0x0, sizeof(tmpDir));
	status = _ttmpnam_s(dirName, MAX_PATH);
	if (status)
	{
		msg(M_ERR | M_ERRNO, L"%s::%d, Error occurred creating unique filename.", TEXT(__FUNCTION__), __LINE__);
	}
	else
	{
		_stprintf_s(tmpDir, ARRAY_SIZE(tmpDir), L"%s\\", dirName);
		msg(M_DEBUG, L"%s::%d, %s is safe to use as a temporary directory.", TEXT(__FUNCTION__), __LINE__, tmpDir);
		result = CreateDirectory(tmpDir, NULL);
		if (!result)
		{
			msg(M_ERR | M_ERRNO, L"%s::%d, Error occurred creating directory %s.", TEXT(__FUNCTION__), __LINE__, tmpDir);
		}
		else
		{
			msg(M_DEBUG, L"%s::%d, %s created.", TEXT(__FUNCTION__), __LINE__, tmpDir);
		}
	}

	return result;
}

BOOL isNotHandle(HANDLE h)
{
	return (h == NULL || h == INVALID_HANDLE_VALUE);
}

BOOL unpackCabinet(void)
{
	BOOL result			= FALSE;
	HRSRC dllCabFile	= INVALID_HANDLE_VALUE;
	HGLOBAL cabResource = INVALID_HANDLE_VALUE;
	HANDLE cabFile		= INVALID_HANDLE_VALUE;
	void* cabData		= NULL;
	
	DWORD cabSize		= 0x0;
	DWORD szWrote		= 0x0;

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	if (!createTmpDir())
	{
		msg(M_ERR | M_ERRNO, L"%s::%d, Error occurred creating directory %s.", TEXT(__FUNCTION__), __LINE__, tmpDir);
	}
	else
	{
		dllCabFile = FindResource(dllInstance, MAKEINTRESOURCE(IDR_CAB), L"BIN");
		if (isNotHandle(dllCabFile))
		{
			msg(M_ERR | M_ERRNO, L"%s::%d, Could not obtain resource handle to CAB file==0x%x!", TEXT(__FUNCTION__), __LINE__, dllCabFile);
		}
		else
		{
			cabResource = LoadResource(dllInstance, dllCabFile);

			if (isNotHandle(cabResource))
			{
				msg(M_ERR | M_ERRNO, L"%s::%d, Could not load resource from handle==0x%x!", TEXT(__FUNCTION__), __LINE__, cabResource);
			}
			{
				cabData = LockResource(cabResource);
				if (!cabData)
				{
					msg(M_ERR | M_ERRNO, L"%s::%d, Could not bind resource binary data==0x%p!", TEXT(__FUNCTION__), __LINE__, cabData);
				}
				else
				{
					_stprintf_s(fileName, ARRAY_SIZE(fileName), L"%sinpoutng.cab", getCabTmpDir());
					cabFile = CreateFile(fileName,
						GENERIC_READ | GENERIC_WRITE,
						0,
						NULL,
						CREATE_ALWAYS,
						0,
						NULL);

					if (isNotHandle(cabFile))
					{
						msg(M_ERR | M_ERRNO, L"%s::%d, Could not create file %s (handle==0x%x)!", TEXT(__FUNCTION__), __LINE__, fileName, cabFile);
					}
					else
					{
						cabSize = SizeofResource(dllInstance, dllCabFile);
						msg(M_DEBUG, L"%s::%d, Created file %s, will write %d bytes to it...!", TEXT(__FUNCTION__), __LINE__, fileName, cabSize);
						result = WriteFile(cabFile, cabData, cabSize, &szWrote, NULL);
						if (!result)
						{
							msg(M_ERR | M_ERRNO, L"%s::%d, Could not write file %s (size %d, wrote %d)!", TEXT(__FUNCTION__), __LINE__, fileName, cabSize, szWrote);
						}
						result &= (cabSize == szWrote);
						if (!result)
						{
							msg(M_ERR | M_ERRNO, L"%s::%d, Wrong number of bytes are written to file %s (size %d != wrote %d)!", TEXT(__FUNCTION__), __LINE__, fileName, cabSize, szWrote);
						}
						result &= CloseHandle(cabFile);
						if (!result)
						{
							msg(M_ERR | M_ERRNO, L"%s::%d, Could not close file %s!", TEXT(__FUNCTION__), __LINE__, fileName);
						}
						else
						{
							GetSystemDirectory(expandDir, ARRAY_SIZE(expandDir));
							_stprintf_s(expandFile, ARRAY_SIZE(expandFile), L"%s\\expand.exe", expandDir);
							msg(M_DEBUG, L"%s::%d, Will execute '%s'...", TEXT(__FUNCTION__), __LINE__, expandFile);

							_stprintf_s(expandParams, ARRAY_SIZE(expandParams), L"-F:* %s %s", fileName, getCabTmpDir());
							msg(M_DEBUG, L"%s::%d, With options '%s'...", TEXT(__FUNCTION__), __LINE__, expandParams);

							_stprintf_s(expandDir, ARRAY_SIZE(expandDir), L"%s %s", expandFile, expandParams);
							msg(M_DEBUG, L"%s::%d, With directory '%s'...", TEXT(__FUNCTION__), __LINE__, expandDir);

							ZeroMemory(&si, sizeof(si));
							si.cb = sizeof(si);
							ZeroMemory(&pi, sizeof(pi));

							// Start the child process. 
							if (!CreateProcess(NULL,   // No module name (use command line)
								expandDir,        // Command line
								NULL,           // Process handle not inheritable
								NULL,           // Thread handle not inheritable
								FALSE,          // Set handle inheritance to FALSE
								0,              // No creation flags
								NULL,           // Use parent's environment block
								NULL,           // Use parent's starting directory 
								&si,            // Pointer to STARTUPINFO structure
								&pi)           // Pointer to PROCESS_INFORMATION structure
								)
							{
								msg(M_ERR | M_ERRNO, L"CreateProcess failed (%d).\n", GetLastError());
								//return;
							}

							// Wait until child process exits.
							WaitForSingleObject(pi.hProcess, INFINITE);

							// Close process and thread handles. 
							CloseHandle(pi.hProcess);
							CloseHandle(pi.hThread);

						}
					}
				}
			}
		}
	}
	return result;
}

BOOL removeTmpDir(void)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	GetSystemDirectory(expandDir, ARRAY_SIZE(expandDir));
	_stprintf_s(expandFile, ARRAY_SIZE(expandFile), L"%s\\cmd.exe", expandDir);
	msg(M_DEBUG, L"%s::%d, Will execute '%s'...", TEXT(__FUNCTION__), __LINE__, expandFile);

	_stprintf_s(expandParams, ARRAY_SIZE(expandParams), L"/c rmdir /q /s %s", getCabTmpDir());
	msg(M_DEBUG, L"%s::%d, With options '%s'...", TEXT(__FUNCTION__), __LINE__, expandParams);

	_stprintf_s(expandDir, ARRAY_SIZE(expandDir), L"%s %s", expandFile, expandParams);
	msg(M_DEBUG, L"%s::%d, With directory '%s'...", TEXT(__FUNCTION__), __LINE__, expandDir);

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));

	// Start the child process. 
	if (!CreateProcess(NULL,   // No module name (use command line)
		expandDir,        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		0,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		msg(M_ERR | M_ERRNO, L"CreateProcess failed (%d).\n", GetLastError());
		//return;
	}

	// Wait until child process exits.
	WaitForSingleObject(pi.hProcess, INFINITE);

	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return (status == ERROR_SUCCESS);
}