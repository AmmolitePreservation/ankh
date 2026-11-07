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
3. The workflow tags the merge commit with the current UTC version and builds with `windeployqt`, bundling the Qt DLLs beside `ankh.exe` together with Qt's LGPL license text.
4. It stamps the version into the exe resource, wraps the folder in the installer, and attaches the installer and its SHA-256 to a GitHub release.
5. Release notes are generated from the commits since the previous tag. The repo keeps no changelog file.

## Installer

Ankh builds its own installer; no third-party installer framework is used. The installer's pages are the same Qt code as the application, so the look, the skins and the libraries are shared.

- `ankhSetup-<version>-win64.exe` is a small Win32 stub that carries the application folder (`ankh.exe`, the Qt DLLs, the license texts) as a compressed payload. It extracts the payload to a temporary folder and starts the extracted `ankh.exe` in install mode, which shows the install pages and copies the files.
- The install path is editable. The default is `%LOCALAPPDATA%\Programs\Ankh`, which needs no elevation; the installer elevates only when the chosen folder requires it.
- Installing writes one per-user registry entry, the uninstall entry for Apps and features, and creates a Start menu shortcut. Nothing else is written outside the install folder.
- Uninstall is `ankh.exe` in uninstall mode, started from that entry. It removes the files and the entry, and leaves games, accounts and profiles alone unless the user chooses otherwise.
- Qt is linked dynamically and ships beside `ankh.exe`; nothing is installed separately.
