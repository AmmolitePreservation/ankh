param(
  [string]$version = '',
  [string]$keyFile = '',
  [string]$target = ''
)

$ErrorActionPreference = 'Stop'

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
if ($version -eq '') {
  $version = (Get-Date).ToUniversalTime().ToString('yyyy.MM.dd.HHmm')
}
if ($keyFile -ne '') {
  $env:ANKH_SIGNING_KEY = (Get-Content $keyFile -Raw).Trim()
}
if ([string]::IsNullOrEmpty($env:ANKH_SIGNING_KEY)) {
  throw 'Give -keyFile or set ANKH_SIGNING_KEY.'
}
if ($target -eq '') {
  $target = (git -C $repositoryRoot rev-parse HEAD)
}

& (Join-Path $repositoryRoot 'package.ps1') -version $version
if (-not $?) {
  throw 'Packaging failed.'
}
& (Join-Path $repositoryRoot 'test.ps1')
if (-not $?) {
  throw 'Tests failed.'
}

$distribution = Join-Path $repositoryRoot 'dist'
$installer = Join-Path $distribution "ankhSetup-$version-win64.exe"
& (Join-Path (Join-Path $repositoryRoot 'tools') 'signRelease.ps1') -installerPath $installer -version $version -outputDirectory $distribution

$previous = gh release list --limit 1 --json tagName --jq '.[0].tagName'
if ($previous) {
  $range = "$previous..$target"
} else {
  $range = $target
}
$lines = git -C $repositoryRoot log --pretty=format:'- %s' $range
$notesPath = Join-Path $distribution 'notes.md'
[IO.File]::WriteAllText($notesPath, ($lines -join "`n"))

Invoke-Native 'gh' @(
  'release', 'create', "v$version",
  $installer, "$installer.sha256",
  (Join-Path $distribution 'manifest.json'), (Join-Path $distribution 'manifest.json.sig'),
  '--repo', 'AmmolitePreservation/ankh',
  '--title', "Ankh $version", '--notes-file', $notesPath, '--target', $target
)
Write-Output "Published Ankh $version"
