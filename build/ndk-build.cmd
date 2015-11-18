@echo off
set NDK_ROOT=%~dp0\..
"%NDK_ROOT%\host-tools\bin\make.exe" -f "%NDK_ROOT%\build\core\build-local.mk" SHELL=cmd %*
