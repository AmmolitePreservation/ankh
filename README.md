# ankh

A minimalist, robust game management application for a private collection of preserved games. It starts as a download-and-play launcher and grows, only for those who want it, into client tools, mixed-game sessions, automation, server-side bots, and MCP access.

- Product and interface specification: [docs/launcherSpec.md](docs/launcherSpec.md)
- Release process and version format: [docs/releases.md](docs/releases.md)
- Update flow: [docs/updates.md](docs/updates.md)
- Installing and updating games: [docs/install.md](docs/install.md)
- Ammolite backend, sign-in and the library's data: [docs/ammolite.md](docs/ammolite.md)
- Wordmark: [assets/ankhWordmark.svg](assets/ankhWordmark.svg)

## Stack

C++20 with Qt 6 (Widgets, Svg, Network), Windows first. Qt is linked dynamically and its DLLs ship beside `ankh.exe`. The installer is built with Inno Setup and installs the Microsoft Visual C++ runtime only when the machine lacks it.

## Build

Required environment variables (machine values, no defaults):

- `ANKH_QT_ROOT`: the Qt MSVC kit, for example `C:\Qt\6.10.3\msvc2022_64`.
- `ANKH_ISCC`: the Inno Setup compiler, for example `C:\Program Files (x86)\Inno Setup 6\ISCC.exe`.

Visual Studio 2022 with the C++ workload is also required.

```
.\build.ps1                      builds build\Release\ankh.exe
.\package.ps1                    builds, stages with windeployqt, and writes dist\ankhSetup-<version>-win64.exe and its .sha256
```

Both take `-version YYYY.MM.DD.HHMM`; without it they use the current UTC time.

## Assets

Vector first. Glyphs live in `assets/glyphs` as SVG with `currentColor` and are recolored at load. The header uses `assets/ankhWordmarkGold.svg` (gradient mark); `assets/ankhWordmark.svg` is the flat original. The edge grain is two transparent PNGs in `assets/textures` built by `tools/makeEdgeGrain.py` and layered over the window. The icon pack and installer images come from `tools/makeInstallerImages.py`.

## Layers

1. Play: catalog, download, import, required setup, update, launch, accounts.
2. Characters and required compatibility.
3. Optional client tools.
4. Sessions.
5. Automation.
6. Server bots.
7. MCP.

Layers 1 to 3 are the product; 4 to 7 are independently enabled features. See the specification, section 17, for the delivery sequence.

## Current state

Built and tested: the shell, Ammolite sign-in and the manifest-driven library, the game page with its edition's packages, game installs and updates (client plus required tools, verified, resumable, receipted; updates diff the receipt against the new listings and honor write-once files), settings (Account, Games, About Ankh, attributions), the installer (per-user or all-users, silent flags), and in-app updating (signed manifest check, verified download, silent install, relaunch). Releases are made by `release.ps1`, run locally or by `.github/workflows/release.yml`.

Launching and repairing games, and optional tools, are not built.

## Tests

`.\test.ps1` runs every test target through ctest (needs a build first). The install tests drive the job against a local HTTP server and real zip fixtures in `tests/fixtures/install`.