[CmdletBinding()]
param(
    [ValidateSet('Editor','Package','Test')][string]$Action='Editor',
    [string]$EngineRoot='',
    [switch]$Packaged
)
$ErrorActionPreference='Stop'
$project=Join-Path $PSScriptRoot 'LiquidGlassDemo.uproject'
. (Join-Path $PSScriptRoot 'Find-Engine.ps1')
$EngineRoot=Resolve-LiquidGlassEngine $EngineRoot
$root=Split-Path $project
$validation=Join-Path $root 'Validation'
New-Item -ItemType Directory -Force -Path $validation | Out-Null
switch($Action){
    'Editor'{
        & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/Build.bat') LiquidGlassDemoEditor Win64 Development "-Project=$project" -WaitMutex -NoUBA -MaxParallelActions=2
        if($LASTEXITCODE -ne 0){throw "Build failed: $LASTEXITCODE"}
    }
    'Package'{
        & (Join-Path $EngineRoot 'Engine/Build/BatchFiles/RunUAT.bat') BuildCookRun "-project=$project" -noP4 -unattended -utf8output -platform=Win64 -clientconfig=Development -build '-ubtargs=-NoUBA -MaxParallelActions=2' -cook -map=/Engine/Maps/Entry -stage -pak -archive "-archivedirectory=$root/Packaged"
        if($LASTEXITCODE -ne 0){throw "Packaging failed: $LASTEXITCODE"}
    }
    'Test'{
        $flavor=if($Packaged){'Packaged'}else{'Editor'}
        $exe=if($Packaged){Join-Path $root 'Packaged/Windows/LiquidGlassDemo/Binaries/Win64/LiquidGlassDemo.exe'}else{Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'}
        $prefix=if($Packaged){''}else{'"'+$project+'" -game '}
        $report=Join-Path $validation $flavor
        $arguments=$prefix+('-windowed -ResX=1440 -ResY=900 -NoSplash -unattended -ExecCmds="Automation RunTests LiquidGlassDemo." -TestExit="Automation Test Queue Empty" -ReportExportPath="{0}" -abslog="{1}\{2}-run.log"' -f $report,$validation,$flavor)
        $started=Get-Date
        $process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
        if(-not $process.WaitForExit(180000)){Stop-Process -Id $process.Id;throw 'Demo test timed out'}
        $json=Join-Path $report 'index.json'
        if(-not(Test-Path -LiteralPath $json)){throw 'Automation report missing'}
        if((Get-Item -LiteralPath $json).LastWriteTime -lt $started){throw 'Automation report stale'}
        $r=Get-Content -LiteralPath $json -Raw | ConvertFrom-Json
        $r | Select-Object succeeded,succeededWithWarnings,failed | ConvertTo-Json -Compress
        $r.tests.entries.event | Where-Object {$_.type -ne 'Info'} | ConvertTo-Json -Depth 4
        if($process.ExitCode -ne 0 -or $r.failed -gt 0 -or $r.succeeded -lt 1){throw 'Demo test failed'}
    }
}
