rem @echo off
rem This is a Windows cmd.exe script used to find the
rem the ndk root and the windows host name.
rem %1 is the env. var to set to the ndk root.
rem %2 is the env. var to set to the windows host name.
rem Uses "endlocal &&" hack to allow env. vars to
rem survive setlocal/endlocal

setlocal
set "TEMP1=%~dp0"

rem Check if %NDK_ROOT% contains any spaces
goto :L
:FOO
  if "%2"=="" goto:EOF
  echo ERROR: NDK path cannot contain any spaces!
  exit /b 1

:L
call :FOO %TEMP1%
if ERRORLEVEL 1 exit /b 1

set TEMP2=windows-x86_64

rem Check if NDK_HOST_32BIT is not set to 1/true, Windows is 64-bit, and 64-bit make exists
if "%NDK_HOST_32BIT%"=="1" set TEMP2=
if "%NDK_HOST_32BIT%"=="true" set TEMP2=
if not exist "%TEMP1%prebuilt\%TEMP2%" set TEMP2=
if "%ProgramW6432%"=="" if "%ProgramFiles(x86)%"=="" set TEMP2=

rem Otherwise fall back to 32-bit make
if "%TEMP2%"=="" set TEMP2=windows

endlocal && set "%1=%TEMP1%" && set "%2=%TEMP2%"

exit /b 0
