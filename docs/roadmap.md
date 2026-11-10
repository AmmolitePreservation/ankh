# Roadmap

The order of work across Ammolite, Ankh, the plugins and the emulators. Each phase ships on its own and leaves everything working; a phase's exit test is what "done" means. `loginModel.md` holds the login design this follows; `launcherSpec.md` the product.

World of Warcraft Classic is the first game through the login model and EverQuest the last: EQEmu's server code is taken by the `guids` and `housing` work, cmangos is free.

## Standing rules

- A game server is modified only in a copy the AmmolitePreservation organization holds: public fork for a GitHub upstream, private repository otherwise.
- Where a server's changes are proven is set per game. EverQuest: on peridothousing and peridotguids, never on Peridot. World of Warcraft: on Arcanite itself, built from the fork's `claude` branch, with no clone and restarts as the work needs. The Phase 5 games are decided at their turn.
- Nothing a production server's players use is removed before that server runs the new model.
- Plugins reach a game folder Ankh manages only as versions of an Ammolite package applied by Ankh; development builds go to a separate copy of the game that Ankh does not manage.
- Group B's TLO work on the DDO tools is not interrupted: the repository rename keeps their clones working and the move to `games/ddo` happens at a point agreed with them.

## Phase 0: names and the plugin repository

- Ammolite: package roles `client`, `requiredPlugin`, `optionalPlugin`; plugin category (Login, Compatibility, Display, Interface, Multiboxing, Research) on every package; migration, admin form, API, manifest.
- Ankh: the Client tools tab becomes Plugins, grouped by category, Research behind a Settings switch; the Play page summary line follows.
- ddo-client-tools renamed to `ankh-plugins`. Layout `core/` (hooks, settings, logs, pipe, loader base, build check, Ammolite client), `plugins/` (generic, written once), `games/<game>/` (loader, offsets, ports, the rare game-specific plugin). DDO moved to `games/ddo` with `git mv`; its build and `package.ps1` produce the same nine DLLs, one package per plugin.
- Exit: DDO installs from the new packages through Ankh and plays as before.

## Phase 1: accounts on Ammolite

- Added beside what exists: account verbs on the server handlers; one account per player per server; `moveCharacter` and the reversible merge that moves a player's characters into that account; the account's random password, set through the handler when the account is created, kept encrypted, and replaced only when access is removed or a reset is asked for; `GET /api/v1/servers/<id>/credential`, which returns that password to the signed-in player.
- The cmangos handler carries the new verbs first (`moveCharacter` is `characters.account`); every other handler gains them in its game's phase.
- Account domains, the claim flow, the account pages, `GET /me/gameAccounts` and Ankh's Account list keep serving every server that has not moved; each server sheds them at its own release.
- Exit: on Arcanite, a player's test accounts merge into one and back again; the password Ammolite returns logs a stock client in, and stops doing so once the player's access is removed.

## Phase 2: World of Warcraft Classic

Running on Arcanite, from the `claude` branch of the organization's mangos-classic fork:

- A session per login with the lock on the character: characters of one account are in the world together, a character already in the world is refused a second time, and a character whose client dropped is taken over by its next login.
- `CharactersPerRealm` and `CharactersPerAccount` accept 50.
- Measured with stock 1.12.1 clients on one account of 50 characters: 40 in the world at once and in one raid group, the server's load average under 0.3. On the client PC a client takes about 365 MB of memory, 105 MB of video memory and 0.2 core at 10 frames per second; 40 of them take 9 of onyx's 16 threads.
- The stock client fails a character list longer than 10 at one compare in its character list handler. A one-byte patch lifts it (thorium's `login` tool, branch `claude`, unreleased). The screen still draws 10 buttons; the rest are reached with the arrow keys.

Left on the server:

- A session key per login in realmd. Today one `account.sessionkey` is overwritten by every login, so a login made before the previous client reaches its character list breaks that client, and an earlier client cannot reconnect.
- The AFK logout (`WorldSession::ShouldAfkDisconnect`): stock cmangos returns a client to the character list 20 minutes after its last input, which empties an idle team. See Open decisions.
- Account state that sessions now share or overwrite: account data and tutorial flags, `active_realm_id`, mute and antispam flags (they reach one session), the one online character the account list commands show (`ShowAccountListHelper`), the script ban in `Player::BanPlayer`.
- Rules keyed by account, each to be decided rather than inherited: bids on the account's own auctions, one guild charter signature per account, the two-faction rule on PvP realms, the bundled PlayerBot's master lookup and same-account tests.
- Login paths not yet exercised or known wrong: a second login for a character whose first is still loading; logins of one account within the same second; the build without playerbots, whose logout marks every character of the account offline and whose login callback leaks the load when the session is gone; the playerbot takeover with no account check.
- Log noise: a `CMSG_CANCEL_TRADE` error line after a refused login or a return from the character list; an offline session's logout logged with "IP: bot".

Left on the client:

- `games/wow`: the 1.12 loader and thorium's tools as generic plugins, thorium's shared code reconciled into `core/`; plus `login` (Login-button credential swap, realm address from the launch, character screen capacity with sort, search and filter) and `sessionReport`.
- The character screen past 10 buttons: scrolling, as part of `login`.
- `WTF\Config.wtf` is shared by every client of an install and rewritten at each Enter World, so a starting client can read another's last character; the per-instance settings files of Phase 3 answer it.
- Ankh: Play for museum games starts the installed client aimed at the chosen server; the Characters tab lists the account's characters and offers launch-to-character.
- Exit: Play, Login with no typing; 50 characters on one account shown and searchable; 40 of them in the world at once and in one raid group; the same character refused a second time; the account-keyed rules behaving as decided.

## Phase 3: multiboxing

- Ankh: character sets (profiles: members, per-member launch-to-character or stop at the screen, placement, plugins), the Sessions page (every running client, state from `sessionReport`, bring to front, close, relaunch), launch-all with staggering and the login queue, healing as scarab's directConnect did it (relaunch a game that dies before it claims its place, end a frozen one after the stability dump, end one the server has gone silent on, retry a refused sign-in while the server still holds a dead session). DDO joins through its saved logins; WoW through Phase 2.
- `plugins/`: `sessionReport` for every game; window layouts (main plus thumbnails, swap on a hotkey, placement saved per member, no focus theft); per-instance settings files redirected by the loader so one install serves the team.
- Exit: the fifty launch from one profile to a layout, the board shows every member in the world, and a crashed member comes back by itself.

## Phase 4: client memory sharing

- `memoryProfile` (Research): per client, what holds memory: system-side texture copies, GPU allocations, heap by site; run on one and ten clients.
- `assetShare` (Multiboxing): Direct3D 9Ex wrapper sharing textures across clients through shared handles, one GPU copy and no system copy; geometry sharing only if the profile shows it matters after that.
- Exit: clients per gigabyte against the Phase 2 baseline, on WoW 1.12 first.

## Phase 5: the other games

In this order:

- Asheron's Call: ACE changes; `games/ac` with the generic plugins, `multibox` and the one named plugin, `scarab`, the ThwargLauncher replacement that keeps what AC players rely on inside the client (`/tf` team commands, ThwargFilter compatibility); directConnect deleted once Ankh covers its launch and swarm.
- Motor City Online: Rusty Motors changes; `games/mco` from clearcoat.
- City of Heroes: Ourodev changes; `games/coh` with the Impervium account service as its handler.
- EverQuest, last: EQEmu changes on the branch the owner places them among `rof2`, `guids` and `housing`: unlimited characters per account; many sessions per account with the lock on the character, in world and login server; the shared bank made safe for two sessions on one account; per-IP caps. `games/eq`: the RoF2 loader and the generic plugins with `login`. Peridot's 72 accounts merge into one.
- Exit per game: the Phase 2 test at that game's scale; for EverQuest a 72-character profile.

## Phase 6: removal

- Once every server Ammolite lists runs the model: `gameAccount` tables, domain remnants and Ankh's domain fields gone; thorium, scarab and clearcoat archived; package descriptions and screenshots filled for every plugin.

## Open decisions

- Whether launch-to-character presses Enter World itself.
- The AFK logout for multiboxed clients: lifted in the server (one condition in `ShouldAfkDisconnect`), or answered by a keep-alive in the `login` plugin.
- Whether the raid test also means entering a raid instance; so far it is a raid group in the open world.
- Where the Phase 5 games' server changes are proven: on the server itself, as with Arcanite, or on a copy.
- When the account model reaches players on each server.
- Talasite, Saronite, Kyber and Carbonite are listed on Ammolite and named in no phase.
