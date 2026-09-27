param([Parameter(Mandatory=$true)][string]$GameDirectory)
$ErrorActionPreference='Stop'
$taskPackage=Split-Path $PSScriptRoot
if(Test-Path -LiteralPath (Join-Path $PSScriptRoot 'scripts\NFSMWActionNitro.asi')){$taskPackage=$PSScriptRoot}
$taskBinary=Join-Path $taskPackage 'scripts\NFSMWActionNitro.asi'
if(-not (Test-Path -LiteralPath $taskBinary)){$taskBinary=Join-Path $taskPackage 'bin\NFSMWActionNitro.asi'}
$taskDefaults=Join-Path (Split-Path $taskBinary) 'NFSMWActionNitro.ini'
$taskRoot=(Resolve-Path -LiteralPath $GameDirectory).Path
$taskExe=Join-Path $taskRoot 'speed.exe'
$taskHash=(Get-FileHash -LiteralPath $taskExe -Algorithm SHA256).Hash
if((Get-Item -LiteralPath $taskExe).Length -ne 6029312 -or $taskHash -notin @('80774C2E5D619B4F120B48D4462896FD504C263399D203A238769CFFDE1D253C','B248271BF8EAC8C9B283B8C95E3ADD672B713BF529B05F1780E58268493B9D06')){throw 'Unsupported executable; nothing installed.'}
$taskRunning=Get-CimInstance Win32_Process -Filter "Name='speed.exe' OR Name='nfsMW.exe'"
foreach($taskProcess in $taskRunning){if(-not $taskProcess.ExecutablePath -or (Split-Path $taskProcess.ExecutablePath) -eq $taskRoot){throw 'Close the game before installation.'}}
if(-not (Test-Path -LiteralPath $taskBinary) -or -not (Test-Path -LiteralPath $taskDefaults)){throw 'Missing ASI or default INI.'}
$taskScripts=Join-Path $taskRoot 'scripts'
$null=New-Item -ItemType Directory -Force -Path $taskScripts
$taskBackup=Join-Path $taskRoot ('ActionNitro-backups\'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
$null=New-Item -ItemType Directory -Force -Path $taskBackup
foreach($taskName in @('NFSMWActionNitro.asi','NFSMWActionNitro.ini')){
 $taskOld=Join-Path $taskScripts $taskName
 if(Test-Path -LiteralPath $taskOld){Copy-Item -LiteralPath $taskOld -Destination $taskBackup}
}
$taskIni=Join-Path $taskScripts 'NFSMWActionNitro.ini'
if(-not (Test-Path -LiteralPath $taskIni)){Copy-Item -LiteralPath $taskDefaults -Destination $taskIni}
else {
 if(-not ('ActionNitroInstallerIni' -as [type])){Add-Type @'
using System.Runtime.InteropServices;
using System.Text;
public static class ActionNitroInstallerIni {
 [DllImport("kernel32",CharSet=CharSet.Unicode)] public static extern uint GetPrivateProfileString(string section,string key,string fallback,StringBuilder value,uint size,string file);
 [DllImport("kernel32",CharSet=CharSet.Unicode)] [return:MarshalAs(UnmanagedType.Bool)] public static extern bool WritePrivateProfileString(string section,string key,string value,string file);
}
'@}
 $taskSection=''
 foreach($taskLine in (Get-Content -LiteralPath $taskDefaults)){
  if($taskLine -match '^\[([^\]]+)\]'){$taskSection=$Matches[1];continue}
  if($taskLine -match '^([^;=#][^=]*)=(.*)$'){
   $taskKey=$Matches[1].Trim();$taskValue=$Matches[2].Trim();$taskBuffer=New-Object Text.StringBuilder 2048
   $null=[ActionNitroInstallerIni]::GetPrivateProfileString($taskSection,$taskKey,'__AN_MISSING__',$taskBuffer,2048,$taskIni)
   if($taskBuffer.ToString() -eq '__AN_MISSING__'){
    if(-not [ActionNitroInstallerIni]::WritePrivateProfileString($taskSection,$taskKey,$taskValue,$taskIni)){throw 'Could not merge INI; original is in backup directory.'}
   }
  }
 }
}
Copy-Item -LiteralPath $taskBinary -Destination (Join-Path $taskScripts 'NFSMWActionNitro.asi') -Force
if((Get-FileHash -LiteralPath $taskBinary).Hash -ne (Get-FileHash -LiteralPath (Join-Path $taskScripts 'NFSMWActionNitro.asi')).Hash){throw 'Installed ASI hash mismatch.'}
Write-Output "Installed ActionNitro. Existing INI values retained. Backup: $taskBackup"
