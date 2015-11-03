@echo off
setlocal
set NDK_ROOT=%~dp0
set SHELL=cmd
"%NDK_ROOT%prebuilt/bin/python.exe" -u "%~dp0ndk-gdb.py" %*
endlocal
