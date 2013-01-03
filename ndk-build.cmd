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

rem Check if 64-bit make exists and Windows is also 64-bit
if "%ProgramW6432%"=="" set NDK_MAKE=%NDK_ROOT%\prebuilt\windows\bin\make.exe
if not exist %NDK_MAKE% set NDK_MAKE=%NDK_ROOT%\prebuilt\windows\bin\make.exe

%NDK_MAKE% -f %NDK_ROOT%build/core/build-local.mk SHELL=cmd %*
