@echo off
rem This is a Windows cmd.exe script used to invoke the NDK-specific GNU Make executable
call "%~dp0find-ndk-root-win-host.cmd" NDK_ROOT NDK_WIN_HOST
if %ERRORLEVEL% neq 0 (exit /b 1)
"%NDK_ROOT%prebuilt/%NDK_WIN_HOST%/bin/make.exe" -f "%NDK_ROOT%build/core/build-local.mk" SHELL=cmd %*
