param([string]$ToolRoot = (Join-Path $env:USERPROFILE 'neurocube-tools'))
$ErrorActionPreference='Stop'
# User-local only. No driver installation or machine PATH changes.
New-Item -ItemType Directory -Path $ToolRoot -Force | Out-Null
$downloads=Join-Path $ToolRoot 'downloads'
New-Item -ItemType Directory -Path $downloads -Force | Out-Null
function Checked-Native { param([scriptblock]$Command); & $Command; if($LASTEXITCODE -ne 0){throw 'Tool command failed'} }
function Download-Archive([string]$Url,[string]$Name,[string]$Target,[string]$ExpectedHash) {
    $archive=Join-Path $downloads $Name
    if(!(Test-Path -LiteralPath $archive)){
        Checked-Native { curl.exe -L --fail --retry 3 --connect-timeout 20 --speed-limit 1024 --speed-time 45 -o $archive $Url }
    }
    $actualHash=(Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash
    if($actualHash -ne $ExpectedHash){throw "Archive hash mismatch: $Name. Remove the incomplete download and retry."}
    if(!(Test-Path -LiteralPath $Target)){
        Expand-Archive -LiteralPath $archive -DestinationPath $Target
    }
    Get-FileHash -LiteralPath $archive -Algorithm SHA256
}
$python=Join-Path $ToolRoot 'venv\Scripts\python.exe'
if(!(Test-Path -LiteralPath $python)){Checked-Native { py -3.12 -m venv (Join-Path $ToolRoot 'venv') }}
Checked-Native { & $python -m pip install -r (Join-Path $PSScriptRoot 'requirements.txt') }
Download-Archive 'https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-mingw-w64-x86_64-arm-none-eabi.zip' 'arm.zip' (Join-Path $ToolRoot 'arm') 'F074615953F76036E9A51B87F6577FDB4ED8E77D3322A6F68214E92E7859888F'
$armMd5=(Get-FileHash -LiteralPath (Join-Path $downloads 'arm.zip') -Algorithm MD5).Hash
if($armMd5 -ne '7426B9EEC8B576F0A524EDE63013C547'){throw 'ARM archive does not match official Content-MD5'}
Download-Archive 'https://github.com/raspberrypi/pico-sdk-tools/releases/download/v2.3.1-0/picotool-2.3.1-x64-win.zip' 'picotool.zip' (Join-Path $ToolRoot 'picotool') '68730BE0813F8F35BE2CCA147CF7F1572662D5DCF0F5BA468E02A6DD9E85DB2B'
Download-Archive 'https://github.com/mstorsjo/llvm-mingw/releases/download/20261006/llvm-mingw-20261006-msvcrt-x86_64.zip' 'llvm.zip' (Join-Path $ToolRoot 'llvm') '24847B37A951C8583C3F7EC061DE2EB87F6394215C8EF3FDC4565ED0835396E1'
$sdk=Join-Path $ToolRoot 'pico-sdk'
if(!(Test-Path -LiteralPath $sdk)){Checked-Native { git clone --branch 2.3.1 --depth 1 https://github.com/raspberrypi/pico-sdk.git $sdk }}
$revision=git -C $sdk rev-parse HEAD
if($revision -ne '079c6f39023649b154152db30f1d781e884879bc'){throw 'Pico SDK revision differs from pinned 2.3.1'}
Checked-Native { git -C $sdk submodule update --init --depth 1 lib/tinyusb }
Write-Output "Ready. Use $python tools/neurocube.py build --native, then build."
