# Roadmap

The order of work across Ammolite, Ankh, the plugins and the emulators. Each phase ships on its own and leaves everything working; a phase's exit test is what "done" means. `loginModel.md` holds the login design this follows; `launcherSpec.md` the product.

World of Warcraft Classic is the first game through the login model and EverQuest the last: EQEmu's server code is taken by the `guids` and `housing` work, cmangos is free.

## Standing rules

- A game server is modified only in a copy the AmmolitePreservation organization holds: public fork for a GitHub upstream, private repository otherwise.
- Where a server's changes are proven is set per game. EverQuest: on peridothousing and peridotguids, never on Peridot. World of Warcraft: on Arcanite itself, built from the fork's `claude` branch, with no clone and restarts as the work needs. The Phase 5 games are decided at their turn.
- Nothing a production server's players use is removed before that server runs the new model.
- Plugins reach a game folder Ankh manages only as versions of an Ammolite package applied by Ankh; development builds go to a separate copy of the game that Ankh does not manage.
- No game gets exclusive fullscreen: every game runs as a borderless window covering its monitor at the monitor's own resolution, refresh rate and HDR state, whatever the game or the player asks for. A game's borderless plugin is required. A game that supports HDR keeps its HDR output borderless; none of today's games does. No external library such as DXVK ships; such projects' approaches are studied, not used.

## Phase 0: names and the plugin repository

- Ammolite: package roles `client`, `requiredPlugin`, `optionalPlugin`; a category on every plugin package (Stability, Display, Login, Portability, Automation, Research); migration, admin form, API, manifest.
- Ankh: the Plugins tab lists a game's plugins under their categories, the Research ones only while the Settings switch is on; the Play page summary line follows.
- The plugins live in two repositories: `ankh-plugins` (public: the loader, the shared source and the plugins that know no online game) and a private one that carries it as a submodule and adds every online game. Layout in both: `core/`, `plugins/` (written once, for every game), `games/<game>/` (settings, the game's handlers for a plugin, the rare plugin only it needs). World of Warcraft, Motor City Online, Asheron's Call and Dungeons & Dragons Online build and run from it.
- Every plugin of a game is an Ammolite package of its own, `<game>-<plugin>`, beside the game's loader, `<game>-loader`. The packages a game cannot run without are required and install with it; every other one installs when the player ticks it.
- Exit: each game installs from its packages through Ankh and plays as before. Done for all four: each was updated through Ankh from the per-plugin packages, which removed the old tool packages' files, and started from its Ankh folder with the installed plugins loaded.
- Found on the way, still open: a failed install, update or plugin change leaves no message while an update is still on offer (the offer's text takes the status line). A fresh World of Warcraft install crashed at its first start on some PCs; the game's part of `stability-guard` fixes it.

## Phase 1: accounts on Ammolite

- Added beside what exists: account verbs on the server handlers; one account per player per server; `moveCharacter` and the reversible merge that moves a player's characters into that account; the account's random password, set through the handler when the account is created, kept encrypted, and replaced only when access is removed or a reset is asked for; `GET /api/v1/servers/<id>/credential`, which returns that password to the signed-in player.
- The cmangos handler carries the new verbs first (`moveCharacter` is `characters.account`); every other handler gains them in its game's phase.
- Players see no game accounts on any server: the claim flow, the login pages and Ankh's Account list for museum games are gone, and `GET /me/gameAccounts` answers no domains for older Ankh releases. Games not yet on the model are fixed as each migrates.
- Exit: on Arcanite, a player's test accounts merge into one and back again; the password Ammolite returns logs a stock client in, and stops doing so once the player's access is removed. Met for the merge: on Arcanite a test player's claimed account's 50 characters moved into the account the site made for them, back, and in again, through a database user that may only change a character's account and the realm's counts; the account's character list shows them after a login with nothing typed. Met for access: the password the site handed out logged in to Arcanite's login server, was refused as banned while the player was disabled on the site, and logged in again once they were enabled, the password unchanged.

## Phase 2: World of Warcraft Classic

Running on Arcanite, from the `claude` branch of the organization's mangos-classic fork:

- A session per login with the lock on the character: characters of one account are in the world together, a character already in the world is refused a second time, and a character whose client dropped is taken over by its next login.
- Seen with two stock clients of one account: Change Realm from the character list in the client holding the older key reconnects without a new login and its character enters the world beside the other's; one character entered from both within the same millisecond reaches the world once, and the other client drops back to its list, with no message, and enters another character.
- `CharactersPerRealm` and `CharactersPerAccount` default to 100, and an account holds characters of both teams on every realm type (`AllowTwoSide.Accounts` defaults to 1): one account holds all of a player's characters, Alliance and Horde.
- `account.sessionkey` holds the keys of the account's last 200 logins, newest first. The world server and realmd's reconnect accept any of them, so a login leaves the account's other clients their keys. Ten stock clients logging one account in within the same second all reach the character list.
- Session keys are hashed on the client's 40 bytes at the world login, the reconnect proof and Warden, and realmd's reconnect goes on to serve the realm list, both as upstream has them. A protocol test run on Arcanite covers a reconnect and a world login by the newest key, the oldest key, a key with a zero top byte and a key that is not in the list.
- `AfkLogout` in `mangosd.conf` switches the logout of a character that has been AFK for 15 minutes. It defaults to 1, stock cmangos, which returns a client to the character list 20 minutes after its last input; Arcanite runs it at 0, so an idle team stays in the world. Checked with a client 17 minutes into AFK, which the setting at 1 then logged out at once; hours of idling are not yet measured.
- Measured with stock 1.12.1 clients on one account of 50 characters: 40 in the world at once and in one raid group, the server's load average under 0.3. On the client PC a client takes about 365 MB of memory, 105 MB of video memory and 0.2 core at 10 frames per second; 40 of them take 9 of onyx's 16 threads. Then one account of 100 characters, 50 Alliance and 50 Horde: its 50 Horde characters in the world at once from 50 clients, the server's load average 0.3 and the world server at 1.5 GB; onyx at full CPU with 7.7 GB in use.
- The stock client fails a character list longer than 10 at one compare in its character list handler. The `login` plugin's one-byte patch lifts it, and its script for the character screen makes the list scroll: 9 rows, moved by the mouse wheel and kept on the selected character, with Create New Character always below them (the stock screen shows Create only while one of its 10 rows is empty). One account's 50 characters were made through that button, and the 30th entered the world.

Left on the server:

- Merge upstream cmangos master. The fork stands at upstream `8ec338a17`; upstream has since reworked and hardened realmd's authentication (`4ad703442`, `2f390f546`, `933d18ee2`, `57cc05969`, `c98484f8b`, `42305c790`). Conflicts to expect in `WorldSocket.cpp`: upstream requires one 80-character key and reconnects to the account's one session, against the key list and a session per login.
- Account state that sessions now share or overwrite: account data and tutorial flags, `active_realm_id`, mute and antispam flags (they reach one session), the one online character the account list commands show (`ShowAccountListHelper`), the script ban in `Player::BanPlayer`.
- Rules keyed by account, each decided rather than inherited. `mangosd.conf` settings, stock by default: `Auction.BidOnOwnAccount` (0: a bid on another character of the account's auction is refused, the seller online or not; Arcanite 0) and `Guild.CharterSignaturesPerCharacter` (0: one charter signature per account; Arcanite 1). Both proven on Arcanite with three characters of one account online, each setting both ways. The playerbots module Arcanite builds (the organization's cmangos-playerbots fork) adds any character of the player's account or guild as a bot with no limit on their number, characters played from their own clients included, and a bot obeys every character of its account: kept as it is.
- Login paths known wrong: the build without playerbots, whose logout marks every character of the account offline and whose login callback leaks the load when the session is gone; the playerbot takeover with no account check.
- Log noise: a `CMSG_CANCEL_TRADE` error line after a refused login or a return from the character list; an offline session's logout logged with "IP: bot".

Left on the client:

- `login` for `games/wow` does the Login-button credential swap, takes the player's Ammolite name and password typed at the login screen (with or without Ankh, retyped after a return to it), and takes a new account straight to its world's character list; still to do: the realm address from the launch and `sessionReport`.
- Search and sort on the character screen: held back by the owner for now.
- `WTF\Config.wtf` is shared by every client of an install and rewritten at each Enter World, so a starting client can read another's last character; the per-instance settings files of Phase 3 answer it.
- Ankh: Play for museum games starts the installed client, and for World of Warcraft Classic hands it the player's sign-in and shows no accounts (seen on the workstation: the client Ankh started reported the site as its credential source; pressing Login in that freshly installed copy waits on the game's first-run terms, which a person accepts). Still to do: the Characters tab lists the account's characters and offers launch-to-character.
- Exit: Play, Login with no typing; 100 characters of both teams on one account, made and selected from the character screen; 50 of them in the world at once and 40 in one raid group; the same character refused a second time; the account-keyed rules behaving as decided.

## Display: every game borderless

Starts once the account work in progress is finished. Learned from DXVK: a game that asks for fullscreen gets a borderless window, and its frames are presented by flipping rather than copying, which Windows treats like fullscreen (full refresh rate, variable refresh, low latency, instant Alt+Tab, mixed-refresh monitors). In order, each step measured before the next:

1. A test setup on onyx that matches a player's: a 21:9 3440x1440 mode and a second monitor, and measurements of the display mode before and after a start, the frame rate before and after Alt+Tab, where the cursor may go, and whether the login screens are shown pixel for pixel.
2. World of Warcraft Classic reproduced with those numbers, as the workstation showed it: the game believes it is fullscreen, so it keeps the cursor on its monitor and loses its frame rate after Alt+Tab; its login and character screens are rendered 3440 wide and squeezed into a 2560-wide picture, which softens their text.
3. The game runs in its own windowed mode; `display-guard` keeps the window borderless, covering the monitor and in front at the start. Keeping the cursor in the window is a `display-guard` setting, off by default.
4. Flip-model presentation in `display-guard` through Direct3D 9Ex, with the game's managed resources translated for it; proven on onyx, its frame rate and Alt+Tab measured, before it ships to any game.
5. The login and character screens letterboxed natively: the game is given a 16:9 screen at the monitor's height there and draws it pixel for pixel into the window's center with black bars around it, mouse positions mapped to match; the world uses the full width.
6. The in-world interface at 21:9 measured, and fixed if it stretches.
7. Verified on a player's PC: ultrawide, three monitors, HDR, 175 Hz.
8. Every game's borderless plugin required, then each game in turn: Asheron's Call (`display-guard`) and Motor City Online (`d3d8-display-guard`), Jade Empire and SOMA checked against the same measurements, Dungeons & Dragons Online's own windowed fullscreen forced by its plugins, and a borderless path for the TBC and WotLK clients, City of Heroes and Star Wars Galaxies. EverQuest is done: `display-guard` is required, its `gameSettings` handler sets the client's fullscreen mode at the monitor's size before each start (the client's own windowed mode fights a borderless window), the game's gamma ramp changes are ignored, and display-guard's window procedure stays first through the login screens' library, which saves and restores the window's procedure and style.

- Exit: on the workstation every game starts in front with no change of display mode, the cursor moves freely between monitors, the frame rate after Alt+Tab matches the one before, and the login screens' text is shown pixel for pixel.

## Phase 3: multiboxing

- Ankh: character sets (profiles: members, per-member launch-to-character or stop at the screen, placement, plugins), the Sessions page (every running client, state from `sessionReport`, bring to front, close, relaunch), launch-all with staggering and the login queue, healing as the `scarab` plugin does it (relaunch a game that dies before it claims its place, end a frozen one after the stability dump, end one the server has gone silent on, retry a refused sign-in while the server still holds a dead session). DDO joins through its saved logins; WoW through Phase 2.
- `plugins/`: `sessionReport` for every game; window layouts (main plus thumbnails, swap on a hotkey, placement saved per member, no focus theft); per-instance settings files redirected by the loader so one install serves the team.
- Exit: the fifty launch from one profile to a layout, the board shows every member in the world, and a crashed member comes back by itself.

## Phase 4: client memory sharing

- `memoryProfile` (Research): per client, what holds memory: system-side texture copies, GPU allocations, heap by site; run on one and ten clients.
- `assetShare` (Multiboxing): Direct3D 9Ex wrapper sharing textures across clients through shared handles, one GPU copy and no system copy; geometry sharing only if the profile shows it matters after that.
- Exit: clients per gigabyte against the Phase 2 baseline, on WoW 1.12 first.

## Phase 5: the other games

In this order:

- Asheron's Call: ACE changes; `login` for `games/ac` beside the named plugin `scarab`, the ThwargLauncher replacement that keeps what AC players rely on inside the client (`/tf` team commands, ThwargFilter compatibility), whose launch and swarm go once Ankh covers them.
- Motor City Online: Rusty Motors changes; `login` for `games/mco`. Done: Fordite's login checks the names and passwords typed at it with Ammolite (a player's Ammolite name and password, or a login key for Ankh's Play), its players' personas are merged into the accounts named after them, Ankh's Play logs the player in with nothing typed, and `database-dao` gives the game Windows' DAO 3.6 for the DAO 3.5 it was built with, so its front end opens its database. Left: a 32-character name typed in the login dialog, several sessions of one account.
- City of Heroes: Ourodev changes; `games/coh` with the Impervium account service as its handler.
- EverQuest: EQEmu changes on the `rof2` branch. Done: Peridot runs it, with its login server checking typed names and passwords with Ammolite (a player's site name and password, or a login key for Ankh's Play), several sessions of one account with the lock on the character, the shared bank as one bank for every session, and the owner's 72 characters merged into one account; 84 clients of one account in one zone at once on a lab copy. `games/eq` is released with the loader, `stability-guard`, `memory-trim` and `login`, and Ammolite starts `eqgame.exe patchme` for Play, so a player presses Play and then Connect with nothing typed (proven on Peridot with MacroQuest running in the client). Also done: names of 3 to 32 letters and digits end to end (the site's EverQuest handler and the world server's account names), no per-IP cap in the way (EQEmu's per-IP limits are world rules, off on Peridot), the license accepted once and remembered, and no game accounts offered by Ammolite. The owner saw all 72 characters after Play on Peridot. Left: the owner's play test of the borderless window on the HDR ultrawide.
- Exit per game: the Phase 2 test at that game's scale; for EverQuest a 72-character profile.

## Phase 6: removal

- Once every server Ammolite lists runs the model: the `gameAccount` table and domain remnants gone; package descriptions and screenshots filled for every plugin.

## Open decisions

- Whether launch-to-character presses Enter World itself.
- Whether the raid test also means entering a raid instance; so far it is a raid group in the open world.
- Where the Phase 5 games' server changes are proven: on the server itself, as with Arcanite, or on a copy.
- When the account model reaches players on each server.
- Talasite, Saronite, Kyber and Carbonite are listed on Ammolite and named in no phase.
