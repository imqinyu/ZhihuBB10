param([ValidateSet('Device-Debug','Simulator-Debug')][string]$Configuration = 'Device-Debug')
$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$env:QNX_HOST = 'C:/NDKFiles/host_10_3_1_12/win32/x86'
$env:QNX_TARGET = 'C:/NDKFiles/target_10_3_1_995/qnx6'
$env:PATH = "$env:QNX_HOST/usr/bin;" + $env:PATH
& "$env:QNX_HOST/usr/bin/make.exe" $Configuration
if ($LASTEXITCODE -ne 0) { throw 'Native build failed' }
New-Item -ItemType Directory -Force -Path dist | Out-Null
$java = 'F:/QDE/bbndk/features/com.qnx.tools.jre.win32.x86_64_1.7.0.51/jre/bin/java.exe'
$lib = "$env:QNX_HOST/usr/lib"
$jars = @('EccpressoJDK15ECC.jar','EccpressoAll.jar','TrustpointAll.jar','TrustpointJDK15.jar','TrustpointProviders.jar','BarPackager.jar','BarSigner.jar','BarDeploy.jar','BarAir.jar')
$classpath = ($jars | ForEach-Object { "$lib/$_" }) -join ';'
$output = "dist/Zhihu-$Configuration.bar"
& $java '-Djava.awt.headless=true' '-Xmx512M' '-cp' $classpath 'com.qnx.bbt.nativepackager.BarNativePackager' '-configuration' $Configuration '-devMode' '-package' $output 'bar-descriptor.xml'
if ($LASTEXITCODE -ne 0) { throw 'BAR packaging failed' }
$archive = [System.IO.Compression.ZipFile]::OpenRead((Join-Path $PSScriptRoot $output))
try {
    if (-not ($archive.Entries | Where-Object FullName -eq 'native/assets/main.qml')) { throw 'main.qml missing from BAR' }
    $archive.Entries | Where-Object { $_.FullName -like '*assets/*.qml' } | Select-Object FullName
} finally { $archive.Dispose() }
Write-Host "Built $output"
