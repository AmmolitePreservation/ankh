param(
  [Parameter(Mandatory = $true)][string]$privateKeyPath,
  [Parameter(Mandatory = $true)][string]$publicKeyPath
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Core

$creation = New-Object System.Security.Cryptography.CngKeyCreationParameters
$creation.ExportPolicy = [System.Security.Cryptography.CngExportPolicies]::AllowPlaintextExport
$key = [System.Security.Cryptography.CngKey]::Create([System.Security.Cryptography.CngAlgorithm]::ECDsaP256, $null, $creation)
$privateBlob = $key.Export([System.Security.Cryptography.CngKeyBlobFormat]::EccPrivateBlob)
$publicBlob = $key.Export([System.Security.Cryptography.CngKeyBlobFormat]::EccPublicBlob)
if ($publicBlob.Length -ne 72) {
  throw "Unexpected public blob length $($publicBlob.Length)."
}
$publicPoint = New-Object byte[] 64
[Array]::Copy($publicBlob, 8, $publicPoint, 0, 64)
[IO.File]::WriteAllText($privateKeyPath, [Convert]::ToBase64String($privateBlob))
[IO.File]::WriteAllText($publicKeyPath, [Convert]::ToBase64String($publicPoint))
$key.Dispose()
