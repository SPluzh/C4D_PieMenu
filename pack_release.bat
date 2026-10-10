@echo off
setlocal enabledelayedexpansion

pushd "%~dp0"

:: Read version from version.txt
if not exist "version.txt" (
    echo [ERROR] version.txt not found!
    exit /b 1
)
set /p VERSION=<version.txt
set VERSION=%VERSION: =%

echo Packaging C4D_PieMenu v%VERSION%...

set "STAGING=dist_staging"
set "ZIP_NAME=C4D_PieMenu_v%VERSION%.zip"

:: Clean old staging and zip
if exist "%STAGING%" rd /s /q "%STAGING%"
if exist "%ZIP_NAME%" del /q "%ZIP_NAME%"

:: Create staging structure
mkdir "%STAGING%\2025\C4D_PieMenu"
mkdir "%STAGING%\2026\C4D_PieMenu"

:: Paths to built plugins
set "SRC_2025=C:\Users\user\Desktop\cpp\C4D_SDK\sdk_2025\build\bin\Release\plugins\C4D_PieMenu"
set "SRC_2026=C:\Users\user\Desktop\cpp\C4D_SDK\sdk_2026\build\bin\Release\plugins\C4D_PieMenu"

:: Copy 2025
if exist "%SRC_2025%" (
    echo Copying 2025 build...
    xcopy /E /I /Y "%SRC_2025%" "%STAGING%\2025\C4D_PieMenu"
) else (
    echo [WARNING] 2025 build not found.
)

:: Copy 2026
if exist "%SRC_2026%" (
    echo Copying 2026 build...
    xcopy /E /I /Y "%SRC_2026%" "%STAGING%\2026\C4D_PieMenu"
) else (
    echo [WARNING] 2026 build not found.
)

:: Create ZIP using PowerShell
echo Creating ZIP archive...
powershell -Command "Compress-Archive -Path '%STAGING%\*' -DestinationPath '%ZIP_NAME%'"

:: Finalize
if exist "%ZIP_NAME%" (
    echo [SUCCESS] Release package created: %ZIP_NAME%
    rd /s /q "%STAGING%"
) else (
    echo [ERROR] Failed to create ZIP archive.
)

pause
popd
