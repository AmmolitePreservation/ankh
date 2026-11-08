# Updates

Ankh updates itself in place from its GitHub releases. Updating is click-only.

## Check

- On start, after signing in or out, every 15 minutes, and from Settings -> About Ankh, the client asks Ammolite for one manifest (`GET /api/v1/manifest`, see `ammolite.md`). Ammolite reads the repository's latest GitHub release for it and keeps the answer for five minutes, so every client together costs GitHub one request. The manifest carries an ETag; an unchanged answer is a 304 and changes nothing.
- The button always reflects the newest release found, and an update sheet that is open but not yet installing switches to a newer release when one appears. A release is newer when its calver tag compares greater than the running version. Equal or older releases are ignored, so a signed older release can never be installed over a newer one.
- A failed check, or a manifest in which Ammolite could not read GitHub, is shown in Settings -> About Ankh. It never blocks starting or playing.

## Button

When a verified update is waiting, a green download button appears in the title bar immediately left of the window controls. Its tooltip names the new version. Clicking it opens a sheet with the version, the release notes and an Update action.

## Install

1. The client downloads `ankhSetup-<version>-win64.exe`, `manifest.json` and `manifest.json.sig` into a private temporary folder.
2. It verifies the manifest signature, then the installer's size and SHA-256 against the manifest. Any mismatch deletes the files and reports the failure.
3. Once Ankh runs jobs of its own (installs, launches), the update waits until none is active and the sheet names what it is waiting for. Today there are no such jobs.
4. It starts the installer silently with the flags of its own scope: `/VERYSILENT /SUPPRESSMSGBOXES /CURRENTUSER /CLOSEAPPLICATIONS /RELAUNCH=1` for a per-user install, `/ALLUSERS` instead of `/CURRENTUSER` for an all-users install. The all-users case raises one UAC prompt.
5. The installer closes Ankh, replaces the files, and starts the new Ankh because of `/RELAUNCH=1`. Running games are never closed.

## Manifest and signature

`manifest.json` holds the version, installer filename, size, SHA-256 and the UTC time it was made. `manifest.json.sig` is a base64 ECDSA P-256 signature (64 bytes, r then s) over the SHA-256 of the manifest's exact bytes, made and checked with Windows' own CNG, so Ankh adds no crypto library. The client verifies the signature first and parses the manifest only after it passes. It then requires the manifest version to equal the release tag and to be newer than the running version, and checks the installer's size and SHA-256 against the manifest.

The private key is the GitHub Actions secret `ANKH_SIGNING_KEY`, readable only by the release workflow on `main`, with a copy in OpenBao at `services/ankh/updateSigningKey`. The public keys the client accepts are listed one per line in `assets/updatePublicKeys.txt` and built into the exe. A key is rotated by shipping a release, signed with the old key, whose build lists the new key too. One key is listed today.

The client downloads updates itself, so Windows does not mark them as downloaded from the internet and SmartScreen is not involved; only a first manual install from the release page can show it. The installer is not Authenticode-signed.