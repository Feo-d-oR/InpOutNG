@echo off
if not exist ..\cab\%2 mkdir ..\cab\%2

copy /Y %1\*.* ..\cab\%2\

if exist ..\cab\x64\inpoutng.inf (
	if exist ..\cab\x86\inpoutng.inf (
		makecab -f inpoutng.ddf
	) else (
		makecab -f inpoutx64.ddf
	)
) else (
		makecab -f inpoutx86.ddf
)

exit 0
