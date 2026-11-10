# Login model: identities, slots and the login module

A design for how a player gets into a museum game, replacing game accounts and account domains. It is a plan, not a description of what exists; `install.md`, `launch.md` and `ammolite.md` say what exists.

## The problem

Every emulator we run (EQEmu, cmangos, ACE, Rusty Motors, Ourodev) lets one client log in per account and keeps characters per account. To have many characters online at once a player today needs many accounts, each with its own name and password, and Ammolite shows them as "game accounts" under "account domains". One player on Peridot has 72 of them. Emulator password storage is weak (cmangos keeps `SHA1(USER:PASS)`, EQEmu an unsalted hash), so a player's own password should never reach an emulator. An unmodified client cannot be made to log in with Ammolite credentials without either weakening Ammolite's rules or changing every emulator.

## The model

- **Identity.** One Ammolite user has one identity per server. It is implicit: signing in to Ammolite is the only login a player does. Access to a server is granted or removed per user on Ammolite.
- **Characters.** The identity's characters on a server, as many as the server allows, any number online at once. Listed by Ammolite from the server (the handlers' `listCharacters` today).
- **Slots.** Underneath, each character lives in an ordinary emulator account that Ammolite owns: created by the server's handler, named by Ammolite, its password replaced with a random one on every login and never known to anyone. Slots exist only because of the one-session-per-account rule; a server without that rule would need one slot. A slot may hold several characters where the emulator allows; Ammolite maps character to slot.
- **Login module.** A client-side tool in every museum game's tool set. At the login screen it signs in to Ammolite (reusing Ankh's token when Ankh is present, its own sign-in overlay otherwise), shows the identity's characters on that server in its own chooser, and on a pick asks Ammolite for the chosen slot's one-time credential and fills the client's login with it. A plain client without the module cannot log in to a museum server; that is the one requirement, and it is what keeps real passwords off the emulators.
- **Servers own their accounts.** The server's handler configuration carries the account operations (`createAccount`, `changePassword`, `listCharacters`), which exist today on the account handlers. Account domains and login servers disappear as concepts; where two worlds share an emulator login server underneath, each keeps its own slots there, named with the server so they cannot collide.
- **Attached games** (Dungeons & Dragons Online) are outside this model: the player's own accounts, saved on the PC (`launch.md`), the game's own login screen.

## What the player sees

- Sign in to Ankh (or to the module's overlay in the game) once.
- Install a game, press Play. The game starts with nobody logged in and the first screen is the player's characters on that server, all of them, with Create. Pick one, and the module logs that character in; the game's own select screen appears with it chosen.
- Press Play again for another client and another character; characters already online are marked.
- Launch straight to a character from the Characters tab or a profile (missions, multiboxing); same path with the pick made outside the game.
- Never: an account name, a game password, a domain, a claim.

## Migration

Each phase ships on its own and leaves the site working.

1. **Ammolite: servers take over accounts.** Move the account handler's verbs into the server handler; delete `accountDomain` and the domain pages, forms and API; keep the connection configuration on the server record. Existing `gameAccount` rows become slots of the server their domain served (Peridot's 72 attach to Peridot). Players lose the account pages; Ankh's Account list and `GET /me/gameAccounts` go with them.
2. **Ammolite: slots and credentials.** Tables for slots (server, owner, emulator account name, characters cached per slot) and a `POST /api/v1/servers/<id>/characters/<name>/credential` that rotates the slot's password through the handler and returns it once, plus `POST .../slots` to make an empty slot for a new character, with a cap per identity. Rotation uses `changePassword`, which every handler has. Slot names: `<ammoliteUser>-<server>-<n>`, trimmed to each emulator's rules.
3. **Shared core: the login module.** In the tools project's common library: Ammolite client (token store as Ankh's, HTTPS), character list, credential request, chooser screen, credential handoff interface a game port implements. Reads the server from the launch (argument or `.ankh` launch file) so one install serves several servers (`eqhost.txt`, `realmlist.wtf` are no longer edited).
4. **WoW Classic port** in thorium: draw the chooser at the login screen, fill account and password fields, select the realm. First real test on Arcanite.
5. **Ankh Play for museum games.** Start the installed client aimed at the chosen server; the Account list is removed from the game page; the Characters tab lists the identity's characters and offers launch-to-character.
6. **EverQuest port.** A loader-based RoF2 tool set (the DDO and thorium pattern; claudelink stays a MacroQuest research tool) carrying the module; login server from the launch, chooser over the client's login screen.
7. **Asheron's Call, Motor City Online, City of Heroes ports** in scarab, clearcoat and Impervium's tools.
8. **Remove the rest:** the claim flow, `gameAccount` tables, Ankh's account domain fields.

Independent of these: multiboxing profiles and the Sessions board (DDO first), window layouts and per-instance settings files in the shared core, and the optional EQEmu change to share the shared bank across an identity's slots.

## Decisions taken

- Login requires the module; no credential sync to emulators.
- Default Play stops at the chooser; launch-to-character is the secondary action.
- Slots are per server; no shared login-server object.
- Passwords rotate on every login, so a used credential is dead.

## Open

- Slot naming where an emulator restricts account names hardest (cmangos: 16 characters, upper-cased).
- Whether the chooser should press Enter World itself after a pick, or leave the player on the game's own screen.
- Per-IP session caps on each server (EQEmu `MaxClientsPerIP`, ACE `MaximumAllowedSessionsPerIPAddress`) for households with many clients.
