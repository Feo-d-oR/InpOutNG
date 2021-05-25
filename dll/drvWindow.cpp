#include "StdAfx.h"
#include "datadll.h"
#include <Windows.h>

BOOL ñreateDrvWindow(
	PCWSTR lpWindowName,
	DWORD dwStyle,
	DWORD dwExStyle = 0,
	int x = CW_USEDEFAULT,
	int y = CW_USEDEFAULT,
	int nWidth = CW_USEDEFAULT,
	int nHeight = CW_USEDEFAULT,
	HWND hWndParent = 0,
	HMENU hMenu = 0
)
{
	HWND m_hwnd = INVALID_HANDLE_VALUE;
	WNDCLASS wc = { 0 };

	wc.lpfnWndProc = DERIVED_TYPE::WindowProc;
	wc.hInstance = GetModuleHandle(NULL);
	wc.lpszClassName = ClassName();

	RegisterClass(&wc);

	m_hwnd = CreateWindowEx(
		dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
		nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(NULL), this
	);

	return (m_hwnd ? TRUE : FALSE);
}