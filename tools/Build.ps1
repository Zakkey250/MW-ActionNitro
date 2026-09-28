$ErrorActionPreference='Stop'
$taskRoot=Split-Path $PSScriptRoot
$null=New-Item -ItemType Directory -Force -Path "$taskRoot\evidence"
$taskBuild=& 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe' -latest -products '*' -find MSBuild\**\Bin\MSBuild.exe
& $taskBuild "$taskRoot\NFSMWActionNitro.vcxproj" /p:Configuration=Release /p:Platform=Win32 /v:minimal /nologo
if($LASTEXITCODE){throw 'ASI build failed'}
& $taskBuild "$taskRoot\NFSMWActionNitro.vcxproj" /p:Configuration=Release /p:Platform=Win32 /p:TestHarness=true /v:minimal /nologo
if($LASTEXITCODE){throw 'Reward test build failed'}
& "$taskRoot\bin\RewardTests.exe"
if($LASTEXITCODE){throw 'Reward tests failed'}
Copy-Item -LiteralPath "$taskRoot\NFSMWActionNitro.ini" -Destination "$taskRoot\bin\NFSMWActionNitro.ini"
foreach($taskTest in @('HudTests','RejectTests','HookTests','RoadTests','LocalizationTests','UpdateTests','TargetTests')){
 & $taskBuild "$taskRoot\NFSMWActionNitro.vcxproj" /p:Configuration=Release /p:Platform=Win32 /p:TestHarness=true "/p:TestName=$taskTest" /v:minimal /nologo
 if($LASTEXITCODE){throw "$taskTest build failed"}
}
& "$taskRoot\bin\TargetTests.exe"
if($LASTEXITCODE){throw 'Target tests failed'}
# Closed stdin prevents the optional native-fixture reader from waiting interactively.
'' | & "$taskRoot\bin\RoadTests.exe"
if($LASTEXITCODE){throw 'Road tests failed'}
& "$taskRoot\bin\HookTests.exe"
if($LASTEXITCODE){throw 'Hook fixture failed'}
& "$taskRoot\bin\HudTests.exe" "$taskRoot\evidence\hud-preview.bmp"
if($LASTEXITCODE){throw 'HUD test failed'}
$taskBeforeLog=if(Test-Path "$taskRoot\bin\NFSMWActionNitro.log"){(Get-Content "$taskRoot\bin\NFSMWActionNitro.log").Count}else{0}
& "$taskRoot\bin\RejectTests.exe" "$taskRoot\bin\NFSMWActionNitro.asi"
if($LASTEXITCODE){throw 'Reject loader failed'}
$taskFreshLog=Get-Content "$taskRoot\bin\NFSMWActionNitro.log" | Select-Object -Skip $taskBeforeLog
if(-not ($taskFreshLog -match 'REJECTED unsupported executable')){throw 'Unsupported-host rejection missing'}
Add-Type -AssemblyName System.Drawing
$taskPreview=[System.Drawing.Image]::FromFile("$taskRoot\evidence\hud-preview.bmp")
try {$taskPreview.Save("$taskRoot\evidence\hud-preview.png",[System.Drawing.Imaging.ImageFormat]::Png)} finally {$taskPreview.Dispose()}
& "$taskRoot\bin\LocalizationTests.exe" "$taskRoot\evidence\locales.bmp"
if($LASTEXITCODE){throw 'Localization tests failed'}
& "$taskRoot\bin\UpdateTests.exe"
if($LASTEXITCODE){throw 'Update tests failed'}
Write-Output 'PASS full build and isolated tests; NOT game runtime acceptance'
