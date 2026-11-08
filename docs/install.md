# Installing games

Install on a game page downloads the edition's client and its required tools from Ammolite, verifies them, and places them in the game library. Optional tools, updates, launching, repair and uninstall are not built yet.

## Library

- Games live under one library folder, by default `clients` beside `ankh.exe`. Settings -> Games shows it and changes it; a changed folder affects new installs, and games already installed stay where they are.
- Each install is one folder, `<game> - <edition label>` (`World of Warcraft - Classic (1.12.1)`), or `<game>` alone when the client serves several editions (`EverQuest`). Characters Windows refuses in a name become `-`.
- Every install folder carries `.ankh\receipt.json`: the client and each tool with its version and SHA-256, and every file the packages placed with its size and CRC-32 from the server's listing. Ankh scans the library's receipts at start and after every install; a world is installed when its edition's client has a receipt. A receipt that cannot be read is listed under Settings -> Games and its game shows as not installed.
- Downloads and extraction happen under `<library>\.staging`. A finished install leaves it empty.

## Install

1. Install opens a sheet: the client and required tools with their versions and download sizes, the destination folder, the bytes to download, the size once installed, what is still needed and the free space on that drive. An all-users install under Program Files cannot write beside itself, so the sheet refuses until a library folder is chosen; Ankh keeps the choice. Change library folder is on the sheet and in Settings -> Games.
2. The install plan is the edition's client plus every package whose role is required tool. It is refused when the client or a required tool has no published version, or when the edition has no client; the Install button is disabled with the reason.
3. The job reads each package's file list (`GET /api/v1/packages/<name>/versions/<version>/files`), downloads each archive to `.staging\<name>-<version>.zip.part` with `Range` resume, checks its SHA-256 against the manifest (a mismatch deletes the file and stops), extracts each archive with Windows' own `tar.exe` into `.staging\<name>-<version>.extract`, renames the client's folder into place, moves the tools' files in, writes the receipt, and deletes the archives.
4. Progress, the current step and a Cancel button sit on the game page's status line; the library row shows the game as installed when the receipt is written. Cancel stops the download or extraction and removes the extraction folders; finished and partial downloads stay in `.staging`, so the next Install resumes. One install runs at a time; other games say so.
5. Failures are reported on the status line and nothing half-installed is left: the install folder exists only once the client is complete, and a tool file that cannot be moved undoes the folder.

While an install runs, the Ankh update sheet holds its Update button, because the installer would close the download half-way.
