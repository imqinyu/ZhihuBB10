$ErrorActionPreference = 'Stop'
Set-Location $PSScriptRoot
$env:QNX_HOST = 'C:/NDKFiles/host_10_3_1_12/win32/x86'
$env:QNX_TARGET = 'C:/NDKFiles/target_10_3_1_995/qnx6'
$env:PATH = "$env:QNX_HOST/usr/bin;" + $env:PATH
& "$env:QNX_HOST/usr/bin/qmake.exe" -spec "$env:QNX_TARGET/usr/share/qt4/mkspecs/blackberry-x86-qcc" CONFIG+=simulator CONFIG+=debug -o Makefile.simulator UiCheck.pro
if ($LASTEXITCODE -ne 0) { throw 'QA qmake failed' }
& "$env:QNX_HOST/usr/bin/make.exe" -f Makefile.simulator
if ($LASTEXITCODE -ne 0) { throw 'QA build failed' }
$java = 'F:/QDE/bbndk/features/com.qnx.tools.jre.win32.x86_64_1.7.0.51/jre/bin/java.exe'
$lib = "$env:QNX_HOST/usr/lib"
$jars = @('EccpressoJDK15ECC.jar','EccpressoAll.jar','TrustpointAll.jar','TrustpointJDK15.jar','TrustpointProviders.jar','BarPackager.jar','BarSigner.jar','BarDeploy.jar','BarAir.jar')
$classpath = ($jars | ForEach-Object { "$lib/$_" }) -join ';'
& $java '-Djava.awt.headless=true' '-Xmx512M' '-cp' $classpath com.qnx.bbt.nativepackager.BarNativePackager -configuration Simulator-Debug -devMode -package UiCheck.bar bar-descriptor.xml
if ($LASTEXITCODE -ne 0) { throw 'QA packaging failed' }
