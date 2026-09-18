function Resolve-LiquidGlassEngine([string]$Requested) {
    if ($Requested) { return $Requested }
    if ($env:UE_ENGINE_ROOT) { return $env:UE_ENGINE_ROOT }
    $installed=Get-ItemProperty 'HKLM:\SOFTWARE\EpicGames\Unreal Engine\5.8' -ErrorAction SilentlyContinue
    if ($installed -and $installed.InstalledDirectory) { return $installed.InstalledDirectory }
    $standard=Join-Path $env:ProgramFiles 'Epic Games/UE_5.8'
    if (Test-Path "$standard/Engine/Binaries/Win64/UnrealEditor.exe") { return $standard }
    throw 'UE 5.8 not found. Set UE_ENGINE_ROOT or pass -EngineRoot with your engine installation folder.'
}