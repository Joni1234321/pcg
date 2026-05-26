@echo off
REM AI-generated convenience wrapper. Not required; equivalent to running the
REM three cmake commands below by hand.
REM Builds pcg and writes a self-contained folder to release\.
setlocal
set PATH=D:\tools\msys64\clang64\bin;D:\tools\msys64\usr\bin;%PATH%
cd /d "%~dp0"

cmake --preset xd                            || exit /b 1
cmake --build build/xd --target pcg          || exit /b 1
if exist release rmdir /s /q release
cmake --install build/xd --prefix release    || exit /b 1

echo.
echo release\ ready. Run release\pcg.exe.
