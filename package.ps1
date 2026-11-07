param(
  [string]$version = ''
)

$ErrorActionPreference = 'Stop'

function Get-RequiredEnvironment {
  param([string]$name)
  $value = [Environment]::GetEnvironmentVariable($name)
  if ([string]::IsNullOrEmpty($value)) {
    throw "Environment variable $name is required."
  }
  return $value
}

function Invoke-Native {
  param([string]$filePath, [string[]]$arguments)
  $previousPreference = $ErrorActionPreference
  $ErrorActionPreference = 'Continue'
  & $filePath @arguments 2>&1 | ForEach-Object { Write-Output "$_" }
  $exitCode = $LASTEXITCODE
  $ErrorActionPreference = $previousPreference
  if ($exitCode -ne 0) {
    throw "$filePath failed with exit code $exitCode."
  }
}

$repositoryRoot = $PSScriptRoot
$qtRoot = Get-RequiredEnvironment 'ANKH_QT_ROOT'
$compilerPath = Get-RequiredEnvironment 'ANKH_ISCC'
if ($version -eq '') {
  $version = (Get-Date).ToUniversalTime().ToString('yyyy.MM.dd.HHmm')
}

& (Join-Path $repositoryRoot 'build.ps1') -version $version
if (-not $?) {
  throw 'Build failed.'
}

$visualStudio = & (Join-Path (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer') 'vswhere.exe') -latest -property installationPath
$env:VCINSTALLDIR = (Join-Path $visualStudio 'VC') + '\'

$parts = $version.Split('.')
$numericVersion = '{0}.{1}.{2}.{3}' -f [int]$parts[0], [int]$parts[1], [int]$parts[2], [int]$parts[3]

$stageDirectory = Join-Path (Join-Path $repositoryRoot 'build') 'stage'
$outputDirectory = Join-Path $repositoryRoot 'dist'
if (Test-Path $stageDirectory) {
  Remove-Item -Recurse -Force $stageDirectory
}
New-Item -ItemType Directory -Force $stageDirectory | Out-Null
New-Item -ItemType Directory -Force $outputDirectory | Out-Null

Copy-Item (Join-Path (Join-Path (Join-Path $repositoryRoot 'build') 'Release') 'ankh.exe') $stageDirectory
Invoke-Native (Join-Path (Join-Path $qtRoot 'bin') 'windeployqt.exe') @(
  '--release', '--no-translations', '--no-opengl-sw', '--no-system-d3d-compiler', '--no-system-dxc-compiler',
  '--dir', $stageDirectory,
  (Join-Path $stageDirectory 'ankh.exe')
)

Invoke-Native $compilerPath @(
  "/DAppVersion=$version",
  "/DNumericVersion=$numericVersion",
  "/DStageDir=$stageDirectory",
  "/DOutputDir=$outputDirectory",
  "/DRepositoryRoot=$repositoryRoot",
  (Join-Path (Join-Path $repositoryRoot 'installer') 'ankh.iss')
)

$installer = Join-Path $outputDirectory "ankhSetup-$version-win64.exe"
$hash = (Get-FileHash -Algorithm SHA256 $installer).Hash.ToLower()
[IO.File]::WriteAllText("$installer.sha256", "$hash  ankhSetup-$version-win64.exe`n")
Write-Output "Packaged $installer"
