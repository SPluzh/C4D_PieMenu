$ErrorActionPreference = "Stop"
$cmake = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe"
$sdkPath = "C:\Users\user\Desktop\cpp\C4D_SDK\sdk_2026"

Write-Host "Running CMake configure for C4D_PieMenu (2026)..."
& $cmake -B "$sdkPath/build" -S "$sdkPath" -G "Visual Studio 17 2022" -A x64 -DCMAKE_GENERATOR_INSTANCE="C:/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools,version=17.14.36915.13"

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "Building C4D_PieMenu (Release 2026)..."
& $cmake --build "$sdkPath/build" --config Release --target C4D_PieMenu

if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$xdl64Path = "$sdkPath\build\bin\Release\plugins\C4D_PieMenu\C4D_PieMenu.xdl64"
if (Test-Path $xdl64Path) {
    Write-Host "Plugin build successful: $xdl64Path"
    $pdbPath = "$sdkPath\build\bin\Release\plugins\C4D_PieMenu\C4D_PieMenu.pdb"
    if (Test-Path $pdbPath) {
        Write-Host "Removing unnecessary .pdb file..."
        Remove-Item -Path $pdbPath -Force
    }
}

Write-Host "Build complete successfully!"
