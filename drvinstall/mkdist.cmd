xcopy /Y ..\inpoutng\x64\%1\drvinstall\ ..\cab\x64\
xcopy /Y ..\inpoutng\%1\drvinstall\ ..\cab\x86\
makecab -f inpoutng.ddf
