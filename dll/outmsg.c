#include "datadll.h"
#include <stdio.h>
#include <stdlib.h>

static TCHAR messageBuffer[MAX_PATH] = { 0x0 };

VOID
outmsg_va (
    const unsigned int flags,
    const TCHAR* format,
    va_list arglist
)
{
    int        charCount =  0;
    /* Output message string. Note: Message strings don't contain line terminators. */
    charCount = _vswprintf_s_l(messageBuffer, MAX_PATH, format, NULL, arglist);
    if (charCount < MAX_PATH)
    {
        _tcscat_s(messageBuffer, MAX_PATH, L"\n");
    }
    OutputDebugString(messageBuffer);

    if ( (flags & M_ERRNO) != 0)
    {
        /* Output system error message (if possible). */
        DWORD dwResult = GetLastError();
        LPTSTR szErrMessage = NULL;
        if (FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS,
            0,
            dwResult,
            0,
            (LPTSTR)&szErrMessage,
            0,
            NULL) && szErrMessage)
        {
            /* Trim trailing whitespace. Set terminator after the last non-whitespace character. This prevents excessive trailing line breaks. */
            for (size_t i = 0, i_last = 0;; i++)
            {
                if (szErrMessage[i])
                {
                    if (!_istspace(szErrMessage[i]))
                    {
                        i_last = i + 1;
                    }
                }
                else
                {
                    szErrMessage[i_last] = 0;
                    break;
                }
            }

            /* Output error message. */

            _stprintf_s(messageBuffer, MAX_PATH, _T("Error 0x%x: %s\n"), dwResult, szErrMessage);
            OutputDebugString(messageBuffer);
            LocalFree(szErrMessage);
        }
        else
        {
            _stprintf_s(messageBuffer, MAX_PATH, _T("Error 0x%x\n"), dwResult);
            OutputDebugString(messageBuffer);
        }
    }
}

VOID
outmsg (
    const unsigned int flags,
    const TCHAR* format,
    ...
)
{
    va_list arglist;
    va_start(arglist, format);
    outmsg_va(flags, format, arglist);
    va_end(arglist);
}
