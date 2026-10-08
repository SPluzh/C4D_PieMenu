@echo off
pushd "%~dp0"
powershell -ExecutionPolicy Bypass -File ".\deploy_2026.ps1"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Deployment failed!
    pause
    exit /b %ERRORLEVEL%
)
pause
popd
