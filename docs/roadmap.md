# Roadmap

The order of work across Ammolite, Ankh, the plugins and the emulators. Each phase ships on its own and leaves everything working; a phase's exit test is what "done" means. `loginModel.md` holds the login design this follows; `launcherSpec.md` the product.

## Standing rules

- A game server is modified only in a copy the AmmolitePreservation organization holds: public fork for a GitHub upstream, private repository otherwise.
- Server changes are proven on a test server (peridothousing, peridotguids) and never on Peridot until released.
- Plugins reach a game folder only as versions of an Ammolite package applied by Ankh.
- Group B's TLO work in ddo-client-tools continues untouched; its commits merge into ankh-plugins until the move is complete.

## Phase 0: names and the plugin repository

- Ammolite: package roles `client`, `requiredPlugin`, `optionalPlugin`; plugin category (Login, Compatibility, Display, Interface, Multiboxing, Research) on every package; migration, admin form, API, manifest.
- Ankh: the Client tools tab becomes Plugins, grouped by category, Research behind a Settings switch; the Play page summary line follows.
- `ankh-plugins` created from ddo-client-tools' full history. Layout `core/` (hooks, settings, logs, pipe, loader base, build check, Ammolite client), `plugins/` (generic, written once), `games/<game>/` (loader, offsets, ports, the rare game-specific plugin). DDO moved to `games/ddo` with `git mv`; its build and `package.ps1` produce the same nine DLLs, one package per plugin.
- Exit: DDO installs from the new packages through Ankh and plays as before; thorium's and scarab's `shared` differences reconciled into `core/`.

## Phase 1: accounts on Ammolite

- Account domains deleted; account verbs on the server handlers; `moveCharacter` added to each handler; one account per player per server; the merge that moves a player's characters into it, reversible; the claim flow, account pages, `GET /me/gameAccounts` and Ankh's Account list removed.
- `POST /api/v1/servers/<id>/credential` rotates the player's account password through the handler and returns it once.
- Exit: on peridothousing, the owner's test accounts merge into one; a credential logs a stock client in once and is dead after.

## Phase 2: EverQuest server and the EverQuest plugin set

- EQEmu (branch per the owner's placement in `rof2`, `guids`, `housing`): unlimited characters per account; many sessions per account with exclusivity per character, in world and login server; account-keyed state reviewed; per-IP caps raised on the test server.
- `games/eq`: RoF2 loader plus `login` (Login-button credential swap, server address from the launch, character screen capacity with sort, search, filter), `stabilityGuard`, `displayGuard`, `sessionReport`.
- Ankh: Play for museum games starts the installed client aimed at the chosen server; the Characters tab lists the account's characters and offers launch-to-character.
- Exit, the 100-character test on peridothousing: Play, Login with no typing, 100 characters shown and searchable, two clients in the world on one account, the same character refused twice, clients launched until onyx's memory is spent with clients per gigabyte recorded, shared bank and mail behaving as one account, a plugin-less client refused.

## Phase 3: multiboxing

- Ankh: character sets (profiles: members, per-member launch-to-character or stop at the screen, placement, plugins), the Sessions page (every running client, state from `sessionReport`, bring to front, close, relaunch), launch-all with staggering and the login queue, healing as scarab's directConnect did it (relaunch a game that dies before it claims its place, end a frozen one after the stability dump, end one the server has gone silent on, retry a refused sign-in while the server still holds a dead session). DDO joins through its saved logins; EverQuest through Phase 2.
- `plugins/`: `sessionReport` for every game; window layouts (main plus thumbnails, swap on a hotkey, placement saved per member, no focus theft); per-instance settings files redirected by the loader so one install serves the team.
- Exit: a 72-character EverQuest profile launches to a layout and the board shows every member in the world; a crashed member comes back by itself.

## Phase 4: client memory sharing

- `memoryProfile` (Research): per client, what holds memory: system-side texture copies, GPU allocations, heap by site; run on one and ten clients.
- `assetShare` (Multiboxing): Direct3D 9Ex wrapper sharing textures across clients through shared handles, one GPU copy and no system copy; geometry sharing only if the profile shows it matters after that.
- Exit: clients per gigabyte against the Phase 2 baseline, on RoF2 first.

## Phase 5: the other games

- WoW Classic: cmangos session and cap changes; `games/wow` built from thorium's tools as generic plugins plus `login`.
- Asheron's Call: ACE changes; `games/ac` with the generic plugins, `multibox` and the one named plugin, `scarab`, the ThwargLauncher replacement that keeps what AC players rely on inside the client (`/tf` team commands, ThwargFilter compatibility); directConnect deleted once Ankh covers its launch and swarm.
- Motor City Online: Rusty Motors changes; `games/mco` from clearcoat.
- City of Heroes: Ourodev changes; `games/coh` with the Impervium account service as its handler.
- Exit per game: the Phase 2 test at that game's scale.

## Phase 6: removal

- `gameAccount` tables, domain remnants and Ankh's domain fields gone; thorium, scarab, clearcoat and ddo-client-tools archived; package descriptions and screenshots filled for every plugin.

## Open decisions

- EQEmu branch placement for Phase 2.
- Whether launch-to-character presses Enter World itself.
- Peridot rollout timing after the test server passes.
