# Updates

Ankh updates itself in place from its GitHub releases. Updating is click-only.

## Check

- On start and once a day the client reads the repository's latest release from the GitHub Releases API. A public repository needs no token.
- A release is newer when its calver tag compares greater than the running version. Equal or older releases are ignored, so a signed older release can never be installed over a newer one.
- A failed check is shown in Settings -> About Ankh and in Activity. It never blocks starting or playing.

## Button

When a verified update is waiting, a green download button appears in the title bar immediately left of the window controls. Its tooltip names the new version. Clicking it opens a sheet with the version, the release notes and an Update action.

## Install

1. The client downloads `ankhSetup-<version>-win64.exe`, `manifest.json` and `manifest.json.sig` into a private temporary folder.
2. It verifies the manifest signature, then the installer's size and SHA-256 against the manifest. Any mismatch deletes the files and reports the failure.
3. It waits until no install, launch or other job is active. If something is running, the sheet names what it is waiting for.
4. It starts the installer silently with the flags of its own scope: `/VERYSILENT /SUPPRESSMSGBOXES /CURRENTUSER /CLOSEAPPLICATIONS /RESTARTAPPLICATIONS` for a per-user install, `/ALLUSERS` instead of `/CURRENTUSER` for an all-users install. The all-users case raises one UAC prompt.
5. The installer replaces the files and restarts Ankh. Running games are never closed.

## Manifest and signature

`manifest.json` holds the version, installer filename, size, SHA-256 and the UTC time it was made. `manifest.json.sig` is an Ed25519 signature over its exact bytes. The release workflow signs with a private key held as a GitHub Actions secret readable only by the release job on `main`. The client holds two public keys, the current and the next, so a key can be rotated without stranding installed copies. The client downloads updates itself, so Windows does not mark them as downloaded from the internet and SmartScreen is not involved; only a first manual install from the release page can show it.
