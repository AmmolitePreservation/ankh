param(
  [string]$version = ''
)

$ErrorActionPreference = 'Stop'

& (Join-Path $PSScriptRoot 'build.ps1') -version $version
if (-not $?) {
  throw 'Build failed.'
}

$qtRoot = [Environment]::GetEnvironmentVariable('ANKH_QT_ROOT')
if ([string]::IsNullOrEmpty($qtRoot)) {
  throw 'Environment variable ANKH_QT_ROOT is required.'
}

$visualStudio = & (Join-Path (Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer') 'vswhere.exe') -latest -property installationPath
$ctest = Join-Path $visualStudio 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\ctest.exe'
$env:PATH = (Join-Path $qtRoot 'bin') + ';' + $env:PATH
& $ctest --test-dir (Join-Path $PSScriptRoot 'build') -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) {
  throw "ctest failed with exit code $LASTEXITCODE."
}
