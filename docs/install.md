# Installing and updating games

Install on a game page downloads the edition's client and its required tools from Ammolite, verifies them, and places them in the game library; Update brings an installed game to the packages' newest versions. Optional tools, launching, repair and uninstall are not built yet.

## Library

- Games live under one library folder, by default `clients` beside `ankh.exe`. Settings -> Games shows it and changes it; a changed folder affects new installs, and games already installed stay where they are.
- Each install is one folder, `<game> - <edition label>` (`World of Warcraft - Classic (1.12.1)`), or `<game>` alone when the client serves several editions (`EverQuest`). Characters Windows refuses in a name become `-`.
- Every install folder carries `.ankh\receipt.json`: the client and each tool with its version and SHA-256, and every file the packages placed with its size, CRC-32 and write-once flag from the server's listing. Ankh scans the library's receipts at start and after every install; a world is installed when its edition's client has a receipt. A receipt that cannot be read is listed under Settings -> Games and its game shows as not installed.
- Downloads and extraction happen under `<library>\.staging`. A finished install leaves it empty.

## Install

1. Install opens a sheet: the client and required tools with their versions and download sizes, the destination folder, the bytes to download, the size once installed, what is still needed and the free space on that drive. An all-users install under Program Files cannot write beside itself, so the sheet refuses until a library folder is chosen; Ankh keeps the choice. Change library folder is on the sheet and in Settings -> Games.
2. The install plan is the edition's client plus every package whose role is required tool. It is refused when the client or a required tool has no published version, or when the edition has no client; the Install button is disabled with the reason.
3. The job reads each package's file list (`GET /api/v1/packages/<name>/versions/<version>/files`), downloads each archive to `.staging\<name>-<version>.zip.part` with `Range` resume, checks its SHA-256 against the manifest (a mismatch deletes the file and stops), extracts each archive with Windows' own `tar.exe` into `.staging\<name>-<version>.extract`, renames the client's folder into place, moves the tools' files in, writes the receipt, and deletes the archives.
4. Progress, the current step and a Cancel button sit on the game page's status line; the library row shows the game as installed when the receipt is written. Cancel stops the download or extraction and removes the extraction folders; finished and partial downloads stay in `.staging`, so the next Install resumes. One install runs at a time; other games say so.
5. Failures are reported on the status line and nothing half-installed is left: the install folder exists only once the client is complete, and a tool file that cannot be moved undoes the folder.

## Update

An installed game needs an update when the receipt's client or a tool has a newer version in the manifest, when a required tool was attached to the edition since the install, or when a tool in the receipt is no longer attached to the edition at all. The Installed section says "Update available", the status line names the changes, and the primary button becomes **Update**; Check for updates asks Ammolite for a fresh manifest. An installed optional tool stays and follows its newest version. If the edition's client is now a different package, the game cannot be updated and the status says to remove the folder and install again.

The update downloads, verifies and extracts only the changed packages, then applies the rule Steam applies, comparing the receipt with the new file listings:

- a file whose listing entry (size and CRC-32) did not change is left alone, whatever its state on disk;
- a file whose entry changed is written over, including any edit of yours;
- a file the package dropped is deleted, as is every file of a package that left the edition;
- a file no package lists (settings the game created, addons, screenshots, logs) is never touched.

A file the package marks **write-once** (set per package on Ammolite, as patterns such as `*.ini`) is written on install and never overwritten: when its entry changes, your copy stays and the status line lists it. That is for shipped files the game saves settings into; a required setting inside such a file is the launcher's job at launch, not a package migration.

The receipt is rewritten last, so an interrupted update leaves the old receipt and pressing Update again redoes the same steps. While an install or update runs, the Ankh update sheet holds its Update button, because the installer would close the download half-way.
