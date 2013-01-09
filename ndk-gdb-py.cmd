@echo off
rem This is a Windows cmd.exe script used to invoke the NDK-specific Python executable
set NDK_ROOT=%~dp0
set NDK_MAKE=%NDK_ROOT%/prebuilt/windows/bin/make.exe

rem Check if %NDK_ROOT% contains space
goto :L
:FOO
  if "%2"=="" goto:EOF
  echo ERROR: NDK path cannot contain space!
  exit /b 1

:L
call :FOO %NDK_ROOT%
if ERRORLEVEL 1 exit /b 1

%NDK_ROOT%\prebuilt\windows\bin\python.exe -u %NDK_ROOT%ndk-gdb.py %*
