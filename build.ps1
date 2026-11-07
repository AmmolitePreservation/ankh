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

function New-CalendarVersion {
  return (Get-Date).ToUniversalTime().ToString('yyyy.MM.dd.HHmm')
}

function Get-InnoVersion {
  param([string]$compilerPath)
  $uninstaller = Join-Path (Split-Path $compilerPath -Parent) 'unins000.exe'
  return (Get-Item $uninstaller).VersionInfo.ProductVersion
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
  $version = New-CalendarVersion
}

$visualStudio = & (Join-Path (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer') 'vswhere.exe') -latest -property installationPath
if ([string]::IsNullOrEmpty($visualStudio)) {
  throw 'Visual Studio was not found.'
}
$cmakeRoot = Join-Path $visualStudio 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin'
$cmake = Join-Path $cmakeRoot 'cmake.exe'
$buildDirectory = Join-Path $repositoryRoot 'build'

Invoke-Native $cmake @(
  '-S', $repositoryRoot,
  '-B', $buildDirectory,
  '-G', 'Visual Studio 17 2022',
  '-A', 'x64',
  "-DCMAKE_PREFIX_PATH=$qtRoot",
  "-DANKH_VERSION=$version",
  "-DANKH_INNO_VERSION=$(Get-InnoVersion $compilerPath)"
)
Invoke-Native $cmake @('--build', $buildDirectory, '--config', 'Release')
Write-Output "Built Ankh $version in $buildDirectory\Release"
