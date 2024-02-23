@echo off
@rem "D:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
mkdir obj 2>NUL
mkdir bin 2>NUL
del bin\*.pdb 2>NUL
cl -arch:AVX -nologo -EHa- -GR- -W4 -Oi -Z7 -Za -Foobj\ windows_main.c kernel32.lib user32.lib gdi32.lib winmm.lib -GS- -Gs9999999 -link -out:bin\main.exe -incremental:no -nodefaultlib -subsystem:windows -stack:0x1000,0x1000 -heap:0,0
if "%1"=="debug" (start windbgx bin\main.exe)
