@echo off
rem This is a Windows cmd.exe script used to invoke the NDK-specific Python executable
call "%~dp0find-win-host.cmd" NDK_WIN_HOST
if ERRORLEVEL 1 (exit /b 1)
set NDK_ROOT=%~dp0
"%NDK_ROOT%prebuilt/%NDK_WIN_HOST%/bin/python.exe" -u "%~dp0ndk-gdb.py" SHELL=cmd %*
