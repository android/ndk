@echo off
rem This is a Windows cmd.exe script used to find windows host name.
rem %1 is the env. var to set to the windows host name.

setlocal
set NDK_ROOT=%~dp0

rem Check if %NDK_ROOT% contains any spaces
goto :L
:FOO
  if "%2"=="" goto:EOF
  echo ERROR: NDK path cannot contain any spaces!
  exit /b 1

:L
call :FOO %NDK_ROOT%
if ERRORLEVEL 1 exit /b 1

set TEMP=windows-x86_64

rem Check if NDK_HOST_32BIT is not set to 1/true, Windows is 64-bit, and 64-bit make exists
if "%NDK_HOST_32BIT%"=="1" set TEMP=
if "%NDK_HOST_32BIT%"=="true" set TEMP=
if not exist "%NDK_ROOT%prebuilt\%TEMP%" set TEMP=
if "%ProgramW6432%"=="" if "%ProgramFiles(x86)%"=="" set TEMP=

rem Otherwise fall back to 32-bit make
if "%TEMP%"=="" set TEMP=windows

rem Uses "endlocal &&" hack to allow env. vars to survive setlocal/endlocal
endlocal && set "%1=%TEMP%"

exit /b 0
