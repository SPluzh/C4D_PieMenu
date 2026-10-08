@echo off
pushd "%~dp0"
powershell -ExecutionPolicy Bypass -File ".\build_2026.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Build failed!
    pause
    exit /b %ERRORLEVEL%
)
echo.
echo [SUCCESS] Build completed.
pause
popd
