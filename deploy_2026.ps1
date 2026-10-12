$source = "C:\Users\user\Desktop\cpp\C4D_SDK\sdk_2026\build\bin\Release\plugins\C4D_PieMenu"
$dest   = "\\vmware-host\Shared Folders\plugins\C4D_PieMenu"

Write-Host "`n[DEPLOYMENT 2026] Deploying C4D_PieMenu to $dest..."

if (-not (Test-Path $dest)) {
    New-Item -ItemType Directory -Path $dest -Force | Out-Null
}

$xdl = Join-Path $dest "C4D_PieMenu.xdl64"
if (Test-Path -LiteralPath $xdl) {
    $tempOld = Join-Path $dest ("C4D_PieMenu.xdl64." + [System.Guid]::NewGuid().ToString().Substring(0,8) + ".old")
    try {
        Rename-Item -LiteralPath $xdl -NewName (Split-Path $tempOld -Leaf) -Force -ErrorAction Stop
        Write-Host "[NOTE] Renamed existing binary to $tempOld"
    } catch {
        try {
            Remove-Item -LiteralPath $xdl -Force -ErrorAction Stop
        } catch {
            Write-Host "[WARN] Could not rename or delete existing binary: $_"
        }
    }
}

Copy-Item -Path "$source\C4D_PieMenu.xdl64" -Destination $dest -Force
$resSrc = Join-Path $PSScriptRoot "res"
if (Test-Path $resSrc) {
    Copy-Item -Path $resSrc -Destination $dest -Recurse -Force
}

Get-ChildItem -LiteralPath $dest -Filter "*.old" -File | ForEach-Object {
    try { Remove-Item -LiteralPath $_.FullName -Force -ErrorAction SilentlyContinue } catch {}
}

Write-Host "[SUCCESS] Plugin successfully deployed!`n"
