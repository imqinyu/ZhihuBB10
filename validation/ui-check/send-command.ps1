param([Parameter(Mandatory=$true)][string[]]$Commands, [string]$Log = 'checks-0.2.1.log')
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$java = 'F:/QDE/bbndk/features/com.qnx.tools.jre.win32.x86_64_1.7.0.51/jre/bin/java.exe'
$deploy = 'C:/NDKFiles/host_10_3_1_12/win32/x86/usr/lib/BarDeploy.jar'
foreach ($command in $Commands) {
    [IO.File]::WriteAllText((Join-Path $PSScriptRoot 'command.txt'), $command)
    & $java -jar $deploy -putFile command.txt data/ui-command -device 192.168.106.129 -password qinyu -package UiCheck.bar | Out-Null
    if ($LASTEXITCODE -ne 0) { throw "Command transfer failed: $command" }
    Start-Sleep -Milliseconds 700
}
& $java -jar $deploy -getFile logs/log $Log -device 192.168.106.129 -password qinyu -package UiCheck.bar | Out-Null
Select-String -Path $Log -Pattern '\[ui-check\]|\[actions\]|\[cache-' | Select-Object -Last (($Commands.Count * 2) + 6) | ForEach-Object Line
