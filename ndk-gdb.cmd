@echo off
setlocal
set PREBUILT_BIN=%~dp0
set SHELL=cmd
"%PREBUILT_BIN%/python.exe" -u "%~dp0ndk-gdb" %*
endlocal
