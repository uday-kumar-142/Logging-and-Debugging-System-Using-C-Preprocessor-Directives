@echo off
setlocal EnableExtensions
title Logging ^& Debugging Demo -- run.bat

rem ===================================================================
rem  run.bat
rem   1. Finds the MinGW-w64 gcc that was installed via winget.
rem   2. Builds all three profiles (dev / errors-only / release).
rem   3. Runs whichever demo you asked for.
rem
rem  Usage:
rem    run.bat          build everything, run the dev demo   (default)
rem    run.bat trap     run dev demo but force LOG_ASSERT to fail
rem    run.bat errors   run the WARN-and-up-only build
rem    run.bat release  run the logging-removed build
rem    run.bat clean    delete the built binaries
rem ===================================================================

rem ---------------------------------------------------------------
rem  1) Make gcc / mingw32-make reachable (no need to restart the
rem     terminal after installing). The install lives under
rem     %LOCALAPPDATA%\Microsoft\WinGet\Packages.
rem ---------------------------------------------------------------
set "GCCPATH="
for /d %%d in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\*WinLibs*") do (
    if exist "%%d\mingw64\bin\gcc.exe" set "GCCPATH=%%d\mingw64\bin\"
)
if not defined GCCPATH (
    for /d %%d in ("%LOCALAPPDATA%\Microsoft\WinGet\Packages\*WinLibs*") do (
        for /r "%%d" %%g in (gcc.exe) do if exist "%%g" set "GCCPATH=%%~dp%%g"
    )
)
if defined GCCPATH set "PATH=%GCCPATH%;%PATH%"

where gcc >nul 2>nul
if errorlevel 1 (
    echo.
    echo  [ERROR] gcc was not found on this machine.
    echo.
    echo  Install it once with:
    echo      winget install -e --id BrechtSanders.WinLibs.POSIX.UCRT
    echo  Then close and reopen this terminal.
    echo.
    exit /b 1
)

rem ---------------------------------------------------------------
rem  2) Parse the command line.
rem ---------------------------------------------------------------
set "CMD=%~1"
if "%CMD%"=="" set "CMD=run"

rem ---------------------------------------------------------------
rem  3) Build the three profiles.
rem ---------------------------------------------------------------
echo =================================================================
echo   Building all profiles with MinGW-w64 GCC ...
echo     dev     : TRACE level + colors + asserts    ^(demo.exe^)
echo     errors  : WARN/ERROR/FATAL only             ^(errors.exe^)
echo     release : logging compiled out entirely     ^(release.exe^)
echo =================================================================
mingw32-make CC=gcc all errors-only release
if errorlevel 1 (
    echo.
    echo  [ERROR] Build failed. See the messages above.
    exit /b 1
)

rem ---------------------------------------------------------------
rem  4) Run whatever the user asked for.
rem ---------------------------------------------------------------
if "%CMD%"=="run" (
    echo.
    echo  ^>^>^> Running the DEV profile: .\demo.exe
    echo  ^>^>^> Full TRACE-to-FATAL logging with colors; ends at LOG_FATAL (exit 1)
    echo       and prints the leak report from dbg_malloc/dbg_free.
    echo.
    .\demo.exe
) else if "%CMD%"=="trap" (
    echo.
    echo  ^>^>^> Running: .\demo.exe trap
    echo  ^>^>^> Forces LOG_ASSERT(x==3) to fail; the process traps with
    echo       exit code 0xC0000409 (via __builtin_trap).
    echo.
    .\demo.exe trap
) else if "%CMD%"=="errors" (
    echo.
    echo  ^>^>^> Running the ERRORS-ONLY profile: .\errors.exe
    echo  ^>^>^> Only WARN/ERROR/FATAL were compiled in.
    echo.
    .\errors.exe
) else if "%CMD%"=="release" (
    echo.
    echo  ^>^>^> Running the RELEASE profile: .\release.exe
    echo  ^>^>^> Every log statement was removed at compile time.
    echo.
    .\release.exe
) else if "%CMD%"=="clean" (
    mingw32-make CC=gcc clean
    echo.
    echo  Cleaned. Build again with:  run.bat
) else (
    echo.
    echo  Usage:
    echo    run.bat          build everything and run the dev demo  (default)
    echo    run.bat trap     run dev demo but force LOG_ASSERT to fail
    echo    run.bat errors   run the WARN-and-up-only build
    echo    run.bat release  run the logging-removed build
    echo    run.bat clean    delete the built binaries
    echo.
    echo  Full guide: README.md
)

endlocal