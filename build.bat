@echo off

rem tcc -impdef {kernel32,user32,gdi32,...}.dll
tcc -DWinMainCRTStartup=_start -nostdlib main.c -lkernel32 -luser32 -ldwmapi -lwinmm -Wl,-subsystem=windows

if "%1"=="test" (
    where /q cl || call vcvars64.bat || goto :error
    cl -nologo -W4 -WX -Z7 -Oi -EHa- -GR- -GS- -Gs0x1000000^
     main.c kernel32.lib user32.lib dwmapi.lib winmm.lib^
     -link -incremental:no -nodefaultlib -subsystem:windows^
     -stack:0x1000000,0x1000000 -heap:0,0
) else if "%1"=="clean" (
    del *.obj *.exe *.pdb 2>nul
)

:end
exit /b
:error
call :end
exit /b 1
