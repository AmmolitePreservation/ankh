# Releases

## Version

`YYYY.MM.DD.HHMM`, always UTC, read from the clock when the release is cut. Nobody types it and no source file stores it.

| Where | Example (15:30 UTC, 7 November 2026) |
|---|---|
| Git tag | `v2026.11.07.1530` |
| Package | `ankh-2026.11.07.1530-win64.zip` |
| Checksum | `ankh-2026.11.07.1530-win64.zip.sha256` |
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
4. It stamps the version into the exe resource, zips the folder, and attaches the zip and its SHA-256 to a GitHub release.
5. Release notes are generated from the commits since the previous tag. The repo keeps no changelog file.

## Package

A plain zip: `ankh.exe`, the Qt DLLs, and the license texts. No installer and no registry entries. Qt is linked dynamically and ships beside the exe; nothing is installed separately.
