# Installing and updating games

Install on a game page downloads the edition's client and its required plugins from Ammolite, verifies them, and places them in the game library; Update brings an installed game to the packages' newest versions. Optional plugins are chosen on the game's Plugins tab. Attached games (plugins added to a copy of the game the player already has) are described below. Launching, repair and uninstalling a game are not built yet.

## Library

- Games live under one library folder, by default `clients` beside `ankh.exe`. Settings -> Games shows it and is the only place that changes it (the `library/root` setting must be a rooted path; anything else stops Ankh at start). A changed library affects new installs only.
- One install can go somewhere else: Choose another folder on the install sheet puts that game in `<chosen folder>\<game> - <edition>` and changes nothing else.
- Ankh remembers every install folder (`library/installs`), so a game outside the library, or one installed before the library changed, is still known after a restart. A remembered folder whose receipt is gone is forgotten.
- Each install is one folder, `<game> - <edition label>` (`World of Warcraft - Classic (1.12.1)`), or `<game>` alone when the client serves several editions (`EverQuest`). Characters Windows refuses in a name become `-`. The client unpacks into that folder itself (`WoW.exe` beside `.ankh`), with no folder of its own inside; Ammolite refuses a client package that holds everything in one folder.
- Every install folder carries `.ankh\receipt.json`: the client and each plugin with its version and SHA-256, and every file the packages placed with its size, CRC-32 and write-once flag from the server's listing. Ankh scans the library's receipts at start and after every install; a world is installed when its edition's client has a receipt. A receipt that cannot be read is listed under Settings -> Games and its game shows as not installed.
- A receipt written by a release before 2026.11.12 calls its plugin list `tools`. Ankh reads it as it stands and writes it back with the field `plugins`, so the install it records stays installed.
- Downloads and extraction happen in `.staging` beside the install folder (`<library>\.staging`, or `<chosen folder>\.staging`), so moving files into place never crosses a drive. A finished install leaves it empty.

## Install

1. Install opens a sheet: the client and required plugins with their versions and download sizes, the destination folder, the bytes to download, the size once installed, what is still needed and the free space on that drive. An all-users install under Program Files cannot write beside itself, so the sheet refuses until another folder is chosen for the game or the library folder is changed under Settings -> Games.
2. The install plan is the edition's client, every required plugin, and the optional plugins ticked on the Plugins tab. It is refused when the client or a required plugin has no published version, or when the edition has no client; the Install button is disabled with the reason.
3. The job reads each package's file list (`GET /api/v1/packages/<name>/versions/<version>/files`), downloads each archive to `.staging\<name>-<version>.zip.part` with `Range` resume, checks its SHA-256 against the manifest (a mismatch deletes the file and stops), extracts each archive with Windows' own `tar.exe` into `.staging\<name>-<version>.extract`, renames the client's folder into place, moves the plugins' files in, writes the receipt, and deletes the archives.
4. Progress, the current step and a Cancel button sit on the game page's status line; the library row shows the game as installed when the receipt is written. Cancel stops the download or extraction and removes the extraction folders; finished and partial downloads stay in `.staging`, so the next Install resumes. One install runs at a time; other games say so.
5. Failures are reported on the status line and nothing half-installed is left: the install folder exists only once the client is complete, and a plugin file that cannot be moved undoes the folder.

## Attached games

Some games in the library are not hosted by Ammolite at all: the manifest's `attachedGames` name a game, an edition, a color and the executable that identifies a copy of it (for Dungeons & Dragons Online, `x64\dndclient64.exe`). The player already has the game; Ankh adds the edition's plugins to it.

- The game page shows "Your own copy of the game" and the primary button reads **Add plugins**. It opens a folder picker; a folder without the executable is refused with the reason. The sheet then lists the required plugins and the optional ones ticked on the Plugins tab, and Ankh adds them with the same job as a plugin change: files are written into the game folder, a file already there with a plugin's name is replaced and reported, and the game's own files are not touched.
- The receipt is `<game folder>\.ankheceipt.json` with `"client": null` and `"attached": {"game", "variant"}`; downloads stage in `<game folder>\.ankh\.staging`, since the folder beside a player's own install is not Ankh's. The folder is remembered under `library/installs` like a game installed outside the library.
- Updates, the Plugins tab and Apply work as for an installed game, minus the client: a plugin with a newer version is updated, one that left the edition is removed. Removing every plugin at once (detaching) is not built yet. The game's own logins and its launch are described in `launch.md`.

## Plugins

The game page's Plugins tab lists the edition's plugins in two columns: a checkbox row for each plugin under the heading of its category (Stability, Display, Login, Portability, Automation, Research, in that order; a category with nothing to list has no heading) and, beside it, a help box with the selected plugin's description and screenshot, which Ammolite supplies (see `ammolite.md`). The client is not listed there.

- Optional Research plugins are listed only while **Show research plugins** is ticked under Settings -> Games (`plugins/showResearch`). One that is installed stays installed while it is not listed.
- A required plugin is always ticked and cannot be unticked. Before the install its row says it installs with the game; afterwards, that it is installed.
- An optional plugin is ticked when installed. Ticking or unticking only stages a change, and the row says "will be installed" or "will be removed"; nothing downloads or is deleted until **Apply changes** and the confirmation sheet that follows, which lists what will be fetched and removed. **Reset** drops the staged choices. Before the game is installed there is no Apply: what is ticked goes along with Install.
- Applying fetches the plugins ticked and not installed, and removes the plugins unticked and installed, with the same rule as an update (the receipt's files of that package are deleted, anything else is never touched). A file the package marks write-once stays on disk, is reported, and leaves the receipt. The client and the other plugins are not updated by a plugin change; a required plugin that is missing is fetched.
- Nothing can be ticked or applied while an install runs. The screenshot is fetched on first view, checked against the SHA-256 Ammolite gives and cached by that hash under the user's cache folder.

## Update

An installed game needs an update when the receipt's client or a plugin has a newer version in the manifest, when a required plugin was attached to the edition since the install, or when a plugin in the receipt is no longer attached to the edition at all. The Installed section says "Update available", the status line names the changes, and the primary button becomes **Update**; Check for updates asks Ammolite for a fresh manifest. An installed optional plugin stays and follows its newest version. If the edition's client is now a different package, the game cannot be updated and the status says to remove the folder and install again.

The update downloads, verifies and extracts only the changed packages, then applies the rule Steam applies, comparing the receipt with the new file listings:

- a file whose listing entry (size and CRC-32) did not change is left alone, whatever its state on disk;
- a file whose entry changed is written over, including any edit of yours;
- a file the package dropped is deleted, as is every file of a package that left the edition, and a folder those deletions leave empty goes with them;
- a file no package lists (settings the game created, addons, screenshots, logs) is never touched.

Clients published before 2026.11.14 unpacked into a folder of their own inside the install (`World of Warcraft - Classic (1.12.1)\World of Warcraft - Arcanite`). When an update brings a client version that unpacks into the install itself, Ankh first moves everything in that folder up, the game's own files (settings, saved games, screenshots, addons) with the packages', reads the receipt's paths without the folder, and removes it. An empty folder already at the top, such as one an older plugin layout left behind, gives way; any other name the install already holds outside that folder stops the update before anything moves.

A file the package marks **write-once** (set per package on Ammolite, as patterns such as `*.ini`) is written on install and never overwritten: when its entry changes, your copy stays and the status line lists it. That is for shipped files the game saves settings into; a required setting inside such a file is the launcher's job at launch, not a package migration.

The receipt is rewritten last, so an interrupted update leaves the old receipt and pressing Update again redoes the same steps. While an install or update runs, the Ankh update sheet holds its Update button, because the installer would close the download half-way.
