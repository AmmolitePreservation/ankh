# Releases

## Version

`YYYY.MM.DD.HHMM`, always UTC, read from the clock when the release is cut. Nobody types it and no source file stores it.

| Where | Example (15:30 UTC, 7 November 2026) |
|---|---|
| Git tag | `v2026.11.07.1530` |
| Installer | `ankhSetup-2026.11.07.1530-win64.exe` |
| Checksum | `ankhSetup-2026.11.07.1530-win64.exe.sha256` |
| Exe version resource | `2026.11.7.1530` |

The tag carries no zone letter, so the time is UTC by rule: the workflow computes it with `date -u +%Y.%m.%d.%H%M`, never from a local zone. Windows' four-number version resource drops leading zeros; the build derives it from the tag.

## Flow

1. Work lands on `claude`. When it is tested, a claude-to-main pull request is merged.
2. A merge to `main` releases automatically when it changes a shipped path:
   - `src/`
   - `assets/`
   - `CMakeLists.txt`, `cmake/`, and the packaging scripts

   A merge that touches only `docs/`, workflows, or other repo files makes no release.
3. The workflow (`.github/workflows/release.yml`) takes the current UTC version, builds and packages with `package.ps1` (Qt installed with a pinned aqtinstall, Inno Setup installed from a pinned, hash-checked download), runs the signature tests, signs the manifest with `tools/signRelease.ps1`, then creates the tag and the release. Qt's LGPL text ships in the application's Attributions page.
4. The release carries the installer, its SHA-256 file, `manifest.json` and `manifest.json.sig`.
5. Release notes are generated from the commits since the previous tag. The repo keeps no changelog file.

## Installer

Inno Setup builds the installer. Its look is brought close to the application's look with the wizard images, colors and fonts; the installer does not use Qt.

- `ankhSetup-<version>-win64.exe` is produced by the release workflow, which stages the application folder (`ankh.exe`, the Qt DLLs, the license texts) and runs the Inno compiler on one script.
- The wizard asks for **all users** (default `C:\Program Files\Ankh`, elevates) or **this user** (default `%LOCALAPPDATA%\Programs\Ankh`, no elevation). The folder is editable in either case.
- Install writes the uninstall entry for Apps and features (machine-wide or per-user to match the scope) and a Start menu shortcut. Nothing else is written outside the install folder.
- Silent installs take Inno's standard flags: `/SILENT`, `/VERYSILENT`, `/SUPPRESSMSGBOXES`, `/DIR=`, `/ALLUSERS`, `/CURRENTUSER`, `/NORESTART`, `/LOG=`, `/TASKS=`, `/LOADINF=` and `/SAVEINF=`. Custom flags are read from the script when needed.
- Uninstall is Inno's `unins000.exe`, with the same silent flags. It leaves games, accounts and profiles alone unless the user chooses otherwise.
- Qt is linked dynamically and ships beside `ankh.exe`.
- The installer carries Microsoft's Visual C++ redistributable and runs it only when the machine lacks it or has an older one. It is the one shared system component: a single runtime for Ankh and Qt, with a UAC prompt on first install.
- `/RELAUNCH=1` starts the new Ankh after a silent install; the updater passes it.
- Inno Setup and Qt are credited on the in-application Attributions page.
