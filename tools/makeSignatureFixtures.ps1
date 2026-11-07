$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Core

$fixtures = Join-Path (Join-Path (Split-Path $PSScriptRoot -Parent) 'tests') 'fixtures'
New-Item -ItemType Directory -Force $fixtures | Out-Null

function Get-PublicPoint {
  param($signer)
  $blob = $signer.Key.Export([System.Security.Cryptography.CngKeyBlobFormat]::EccPublicBlob)
  $point = New-Object byte[] 64
  [Array]::Copy($blob, 8, $point, 0, 64)
  return $point
}

$signer = New-Object System.Security.Cryptography.ECDsaCng(256)
$other = New-Object System.Security.Cryptography.ECDsaCng(256)
$message = [Text.Encoding]::UTF8.GetBytes('{"version":"2026.01.01.0000","installer":"ankhSetup-2026.01.01.0000-win64.exe","size":1,"sha256":"00"}')
$signature = $signer.SignData($message, [System.Security.Cryptography.HashAlgorithmName]::SHA256)

[IO.File]::WriteAllBytes((Join-Path $fixtures 'message.json'), $message)
[IO.File]::WriteAllText((Join-Path $fixtures 'message.json.sig'), [Convert]::ToBase64String($signature))
[IO.File]::WriteAllText((Join-Path $fixtures 'signerPublic.b64'), [Convert]::ToBase64String((Get-PublicPoint $signer)))
[IO.File]::WriteAllText((Join-Path $fixtures 'otherPublic.b64'), [Convert]::ToBase64String((Get-PublicPoint $other)))
$signer.Dispose()
$other.Dispose()
Write-Output "Wrote fixtures to $fixtures"
