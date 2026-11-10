# Roadmap

The order of work across Ammolite, Ankh, the plugins and the emulators. Each phase ships on its own and leaves everything working; a phase's exit test is what "done" means. `loginModel.md` holds the login design this follows; `launcherSpec.md` the product.

World of Warcraft Classic is the first game through the login model and EverQuest the last: EQEmu's server code is taken by the `guids` and `housing` work, cmangos is free.

## Standing rules

- A game server is modified only in a copy the AmmolitePreservation organization holds: public fork for a GitHub upstream, private repository otherwise.
- Server changes are proven on a test server and never on the production one until released. EverQuest has peridothousing and peridotguids; every other game gets a clone of its server before its phase starts.
- Nothing a production server's players use is removed before that server runs the new model.
- Plugins reach a game folder Ankh manages only as versions of an Ammolite package applied by Ankh; development builds go to a separate copy of the game that Ankh does not manage.
- Group B's TLO work on the DDO tools is not interrupted: the repository rename keeps their clones working and the move to `games/ddo` happens at a point agreed with them.

## Phase 0: names and the plugin repository

- Ammolite: package roles `client`, `requiredPlugin`, `optionalPlugin`; plugin category (Login, Compatibility, Display, Interface, Multiboxing, Research) on every package; migration, admin form, API, manifest.
- Ankh: the Client tools tab becomes Plugins, grouped by category, Research behind a Settings switch; the Play page summary line follows.
- ddo-client-tools renamed to `ankh-plugins`. Layout `core/` (hooks, settings, logs, pipe, loader base, build check, Ammolite client), `plugins/` (generic, written once), `games/<game>/` (loader, offsets, ports, the rare game-specific plugin). DDO moved to `games/ddo` with `git mv`; its build and `package.ps1` produce the same nine DLLs, one package per plugin.
- Exit: DDO installs from the new packages through Ankh and plays as before.

Alongside, the ground Phase 2 stands on:

- A WoW test realm: a clone of arcanite, as peridothousing is of Peridot. None exists today.
- Two spikes on it, before anything is built on their answers: a 1.12 client whose character screen shows more than ten characters, and two clients in the world at once on one cmangos account.

## Phase 1: accounts on Ammolite

- Added beside what exists: account verbs on the server handlers; one account per player per server; `moveCharacter` and the reversible merge that moves a player's characters into that account; `POST /api/v1/servers/<id>/credential`, which returns the account's password for the current login window: set through the handler by the first login to ask, shared by every login that asks while it lives, and scrambled once a minute passes with no login asking.
- The cmangos handler carries the new verbs first (`moveCharacter` is `characters.account`); every other handler gains them in its game's phase.
- Account domains, the claim flow, the account pages, `GET /me/gameAccounts` and Ankh's Account list keep serving every server that has not moved; each server sheds them at its own release.
- Exit: on the WoW test realm, a player's test accounts merge into one and back again; a credential logs two stock clients in inside its window and is dead a minute after the last of them asked.

## Phase 2: World of Warcraft Classic

- cmangos, in the organization's mangos-classic fork:
  - sessions held per login with the lock on the character (today `World::m_sessions` holds one session per account and a second login kicks the first);
  - a session key per login in realmd (today one `account.sessionkey`, overwritten by every login);
  - the ten-per-realm character limit lifted (`CharactersPerRealm` is clamped to 10);
  - every rule keyed by account decided rather than inherited: bids on the account's own auctions, one guild charter signature per account, tutorial flags, the two-faction rule on PvP realms, bans and mutes, online counts, the bundled PlayerBot's same-account test.
- `games/wow`: the 1.12 loader and thorium's tools as generic plugins, thorium's shared code reconciled into `core/`; plus `login` (Login-button credential swap, realm address from the launch, character screen capacity with sort, search and filter) and `sessionReport`.
- Ankh: Play for museum games starts the installed client aimed at the chosen server; the Characters tab lists the account's characters and offers launch-to-character.
- Exit, the 50-character test on the test realm: Play, Login with no typing; 50 characters on one account shown and searchable; 40 of them in the world at once and in one raid group; the same character refused a second time; the other ten launched as far as onyx's memory carries, with clients per gigabyte recorded; the account-keyed rules behaving as decided; a client without the plugins refused.

## Phase 3: multiboxing

- Ankh: character sets (profiles: members, per-member launch-to-character or stop at the screen, placement, plugins), the Sessions page (every running client, state from `sessionReport`, bring to front, close, relaunch), launch-all with staggering and the login queue, healing as scarab's directConnect did it (relaunch a game that dies before it claims its place, end a frozen one after the stability dump, end one the server has gone silent on, retry a refused sign-in while the server still holds a dead session). DDO joins through its saved logins; WoW through Phase 2.
- `plugins/`: `sessionReport` for every game; window layouts (main plus thumbnails, swap on a hotkey, placement saved per member, no focus theft); per-instance settings files redirected by the loader so one install serves the team.
- Exit: the fifty launch from one profile to a layout, the board shows every member in the world, and a crashed member comes back by itself.

## Phase 4: client memory sharing

- `memoryProfile` (Research): per client, what holds memory: system-side texture copies, GPU allocations, heap by site; run on one and ten clients.
- `assetShare` (Multiboxing): Direct3D 9Ex wrapper sharing textures across clients through shared handles, one GPU copy and no system copy; geometry sharing only if the profile shows it matters after that.
- Exit: clients per gigabyte against the Phase 2 baseline, on WoW 1.12 first.

## Phase 5: the other games

In this order, each with its test server cloned first:

- Asheron's Call: ACE changes; `games/ac` with the generic plugins, `multibox` and the one named plugin, `scarab`, the ThwargLauncher replacement that keeps what AC players rely on inside the client (`/tf` team commands, ThwargFilter compatibility); directConnect deleted once Ankh covers its launch and swarm.
- Motor City Online: Rusty Motors changes; `games/mco` from clearcoat.
- City of Heroes: Ourodev changes; `games/coh` with the Impervium account service as its handler.
- EverQuest, last: EQEmu changes on the branch the owner places them among `rof2`, `guids` and `housing`: unlimited characters per account; many sessions per account with the lock on the character, in world and login server; the shared bank made safe for two sessions on one account; per-IP caps. `games/eq`: the RoF2 loader and the generic plugins with `login`. Peridot's 72 accounts merge into one.
- Exit per game: the Phase 2 test at that game's scale; for EverQuest a 72-character profile.

## Phase 6: removal

- Once every server Ammolite lists runs the model: `gameAccount` tables, domain remnants and Ankh's domain fields gone; thorium, scarab and clearcoat archived; package descriptions and screenshots filled for every plugin.

## Open decisions

- Whether launch-to-character presses Enter World itself.
- Release timing for each production server after its test server passes.
- Talasite, Saronite, Kyber and Carbonite are listed on Ammolite and named in no phase.
