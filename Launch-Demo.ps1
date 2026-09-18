[CmdletBinding()]
param([string]$EngineRoot='',[switch]$Editor)
$ErrorActionPreference='Stop'
$project=Join-Path $PSScriptRoot 'LiquidGlassDemo.uproject'
$packaged=Join-Path $PSScriptRoot 'Packaged/Windows/LiquidGlassDemo.exe'
if((-not $Editor) -and (Test-Path -LiteralPath $packaged)){
    Start-Process -FilePath $packaged -ArgumentList '-windowed -ResX=1440 -ResY=900' -WorkingDirectory (Split-Path $packaged)
    exit
}
. (Join-Path $PSScriptRoot 'Find-Engine.ps1')
$EngineRoot=Resolve-LiquidGlassEngine $EngineRoot
$exe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
if(-not(Test-Path -LiteralPath $exe)){throw 'UE 5.8 was not found. Pass -EngineRoot or open LiquidGlassDemo.uproject manually.'}
$options=if($Editor){''}else{' -game -windowed -ResX=1440 -ResY=900'}
Start-Process -FilePath $exe -ArgumentList ('"'+$project+'"'+$options) -WorkingDirectory $PSScriptRoot
