param(
  [Parameter(Mandatory = $true)][string]$installerPath,
  [Parameter(Mandatory = $true)][string]$version,
  [Parameter(Mandatory = $true)][string]$outputDirectory
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Core

$privateKeyText = ([Environment]::GetEnvironmentVariable('ANKH_SIGNING_KEY')).Trim()
if ([string]::IsNullOrEmpty($privateKeyText)) {
  throw 'Environment variable ANKH_SIGNING_KEY is required.'
}

$installer = Get-Item $installerPath
$hash = (Get-FileHash -Algorithm SHA256 $installer.FullName).Hash.ToLower()
$manifest = [ordered]@{
  version = $version
  installer = $installer.Name
  size = $installer.Length
  sha256 = $hash
  created = (Get-Date).ToUniversalTime().ToString('yyyy-MM-ddTHH:mm:ssZ')
}
$manifestBytes = [Text.Encoding]::UTF8.GetBytes(($manifest | ConvertTo-Json -Compress))
$manifestPath = Join-Path $outputDirectory 'manifest.json'
[IO.File]::WriteAllBytes($manifestPath, $manifestBytes)

$key = [System.Security.Cryptography.CngKey]::Import([Convert]::FromBase64String($privateKeyText), [System.Security.Cryptography.CngKeyBlobFormat]::EccPrivateBlob)
$signer = New-Object System.Security.Cryptography.ECDsaCng($key)
$signature = $signer.SignData($manifestBytes, [System.Security.Cryptography.HashAlgorithmName]::SHA256)
if ($signature.Length -ne 64) {
  throw "Unexpected signature length $($signature.Length)."
}
[IO.File]::WriteAllText((Join-Path $outputDirectory 'manifest.json.sig'), [Convert]::ToBase64String($signature))
$signer.Dispose()
$key.Dispose()
Write-Output "Signed manifest for $version"
