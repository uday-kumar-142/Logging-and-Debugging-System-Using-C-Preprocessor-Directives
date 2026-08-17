@echo off
rem =====================================================================
rem  run.bat  - build the demo and run whichever profile you pick.
rem
rem    run.bat          default: build all + run the DEV demo
rem    run.bat trap     DEV demo but force LOG_ASSERT to fail (traps)
rem    run.bat errors   run the WARN-and-up-only build
rem    run.bat release  run the logging-removed build
rem    run.bat clean    delete the built binaries
rem =====================================================================

setlocal EnableExtensions
title Logging ^& Debugging Demo

rem ---------------------------------------------------------------
rem  1) Make MinGW gcc reachable without restarting the terminal.
rem     winget installs it under %LOCALAPPDATA%\Microsoft\WinGet.
rem ---------------------------------------------------------------
set "MINGW_BIN="
for /d %%d in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\*WinLibs*") do (
    if exist "%%d\mingw64\bin\gcc.exe" set "MINGW_BIN=%%d\mingw64\bin\"
)
if defined MINGW_BIN set "PATH=%MINGW_BIN%;%PATH%"

where gcc >nul 2>nul
if errorlevel 1 goto nocompiler

set "CMD=%~1"
if not defined CMD set "CMD=run"

rem ---------------------------------------------------------------
rem  2) Build the three profiles from the same source.
rem ---------------------------------------------------------------
echo =================================================================
echo   Building all profiles with MinGW-w64 GCC ...
echo     dev     : TRACE + colors + asserts    (demo.exe)
echo     errors  : WARN/ERROR/FATAL only       (errors.exe)
echo     release : logging removed entirely    (release.exe)
echo =================================================================

mingw32-make CC=gcc all errors-only release
if errorlevel 1 goto buildfailed

rem ---------------------------------------------------------------
rem  3) Dispatch to the requested demo.
rem ---------------------------------------------------------------
if "%CMD%"=="run"     goto run_dev
if "%CMD%"=="trap"    goto run_trap
if "%CMD%"=="errors"  goto run_errors
if "%CMD%"=="release" goto run_release
if "%CMD%"=="clean"   goto run_clean
goto usage

:run_dev
echo.
echo  ^>^>^> Running the DEV profile: .\demo.exe
echo  ^>^>^> Full TRACE-to-FATAL logging; ends at LOG_FATAL (exit 1)
echo       and prints the leak report from dbg_malloc/dbg_free.
echo.
.\demo.exe
goto :eof

:run_trap
echo.
echo  ^>^>^> Running: .\demo.exe trap
echo  ^>^>^> Forces LOG_ASSERT(x==3) to fail; the process traps with
echo       exit code 0xC0000409 (via __builtin_trap).
echo.
.\demo.exe trap
echo.
echo  ^>^>^> Finished. Exit code 0xC0000409 means the assert trap fired.
goto :eof

:run_errors
echo.
echo  ^>^>^> Running the ERRORS-ONLY profile: .\errors.exe
echo  ^>^>^> Only WARN/ERROR/FATAL were compiled in.
echo.
.\errors.exe
goto :eof

:run_release
echo.
echo  ^>^>^> Running the RELEASE profile: .\release.exe
echo  ^>^>^> Every log statement was removed at compile time.
echo.
.\release.exe
goto :eof

:run_clean
mingw32-make CC=gcc clean
echo.
echo  Cleaned. Build again with:  run.bat
goto :eof

:usage
echo.
echo  Usage:
echo    run.bat          build everything and run the dev demo   (default)
echo    run.bat trap     run dev demo but force LOG_ASSERT to fail
echo    run.bat errors   run the WARN-and-up-only build
echo    run.bat release  run the logging-removed build
echo    run.bat clean    delete the built binaries
echo.
echo  Full guide: README.md
echo.
goto :eof

:nocompiler
echo.
echo  [ERROR] gcc was not found on this machine.
echo.
echo  Install it once with:
echo      winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
echo  Then reopen your terminal.
echo.
exit /b 1

:buildfailed
echo.
echo  [ERROR] Build failed. See the messages above.
exit /b 1