@echo off
rem This is a Windows cmd.exe script used to invoke the NDK-specific GNU Make executable
set NDK_ROOT=%~dp0
set NDK_MAKE=%NDK_ROOT%\prebuilt\windows-x86_64\bin\make.exe

rem Check if %NDK_ROOT% contains space
goto :L
:FOO
  if "%2"=="" goto:EOF
  echo ERROR: NDK path cannot contain space!
  exit /b 1

:L
call :FOO %NDK_ROOT%
if ERRORLEVEL 1 exit /b 1

rem Check if NDK_HOST_32BIT is not set to 1/true, Windows is 64-bit, and 64-bit make exists
if "%NDK_HOST_32BIT%"=="1" set NDK_MAKE=
if "%NDK_HOST_32BIT%"=="true" set NDK_MAKE=
if not exist "%NDK_MAKE%" set NDK_MAKE=
if "%ProgramW6432%"=="" set NDK_MAKE=

rem Otherwise fall back to 32-bit make
if "%NDK_MAKE%"=="" set NDK_MAKE=%NDK_ROOT%\prebuilt\windows\bin\make.exe

%NDK_MAKE% -f %NDK_ROOT%build/core/build-local.mk SHELL=cmd %*
