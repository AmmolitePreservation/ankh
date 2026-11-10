# Login model: one account per server, the login module, and the emulator changes

A plan for how a player gets into a museum game, replacing game accounts and account domains. It is a plan, not a description of what exists; `install.md`, `launch.md` and `ammolite.md` say what exists.

## The problem

Every emulator we run (EQEmu, cmangos, ACE, Rusty Motors, Ourodev) lets one client log in per account, caps the characters an account may hold at what its client's select screen can show (RoF2 8, WoW 1.12 10, Asheron's Call 11, City of Heroes 12), and keeps characters per account. A player who wants many characters online at once therefore needs many accounts, each with a name and a password, and Ammolite today presents them as "game accounts" under "account domains"; one player on Peridot has 72. Emulator password storage is weak (cmangos keeps `SHA1(USER:PASS)`, EQEmu an unsalted hash), so a player's own password must never reach an emulator.

## The model

- **One account per player per server.** The emulator account is the player's identity on that server, created and owned by Ammolite; its password is random, replaced by Ammolite for every login window, and never known to anyone. All of the player's characters on that server live in it. Access is granted or removed per Ammolite user.
- **The server stays the source of the character list.** The client logs in, receives its characters from the server as it always did, renders them, and the player picks one. Nothing of ours draws a list.
- **The login module** is a required client-side tool in every museum game's tool set. It does three things and no more:
  1. When the player presses the game's own Login button it signs in to Ammolite (reusing Ankh's token when Ankh runs, its own sign-in overlay otherwise), obtains the account's short-lived credential, and completes the client's login with it.
  2. It lifts the client's character-screen capacity to an arbitrary count and adds sorting, search and a filter over the list the server sent.
  3. It carries the server address from the launch (argument or launch file) so one install serves several servers without editing `eqhost.txt` or `realmlist.wtf`.
- **The servers allow it.** Each emulator is changed, in our own fork, to hold any number of characters per account and to accept any number of sessions on one account at once, with exclusivity moved to the character (a character can be in the world once). Everything a server keys by account rather than character is reviewed: WoW's account bank and mail, EverQuest's shared bank, bans, account-wide flags and online counts stay account-wide, which is now what the player wants.
- **Servers own their accounts.** The server's handler configuration carries the account operations (`createAccount`, `changePassword`, `listCharacters`, and a new `moveCharacter`); account domains and login servers disappear as concepts.
- **Attached games** (Dungeons & Dragons Online) stay outside this model: the player's own accounts saved on the PC (`launch.md`), the game's own login screen, no server changes.

## What the player sees

- Sign in to Ankh once (or to the module's overlay in the game).
- Install a game and press Play. The client starts at its own login screen, aimed at the chosen server. Press Login; nothing to type.
- The game's own character screen shows every character, searchable and sortable. Pick one, play.
- Press Play again for another client and another character, as many as the machine carries.
- Launch straight to a character from the Characters tab or a profile, for missions and multiboxing: the same path with the pick made in advance.
- Never: an account name, a game password, a domain, a claim, or a choice of account.

## Before any server change

The AmmolitePreservation organization holds a copy of every server we modify: a public fork for an upstream on GitHub, a private repository for any other origin. Today: EQEmu, mangos-classic, acemulator and rustymotors are public forks; ourodev-source is a private copy. Work on a server stops until its copy exists.

## Migration

`roadmap.md` holds the order: World of Warcraft Classic first, EverQuest last, each game proven on a test server before its production server changes. The pieces:

- **Ammolite, one account per server.** The account verbs move onto the server handler and the server record keeps the connection configuration. Each handler gains `moveCharacter` (cmangos `characters.account`, EQEmu `character_data.account_id`, and the equivalents), and a merge moves every character of a player's old game accounts into one account per server, reversible by the same verb. Peridot's 72 become one when EverQuest's turn comes.
- **Ammolite, credentials.** `POST /api/v1/servers/<id>/credential` returns the player's account password for the current login window. The first login to ask has Ammolite set a fresh random password through the handler; every login that asks while it lives receives the same one, so many clients can log in together; once a minute passes with no login asking, Ammolite scrambles it. An emulator holds one password per account, so a password replaced on every login would cancel the logins still in flight.
- **Each emulator, in the organization's copy:** characters per account unlimited; several sessions per account with the lock per character; every rule the server keys by account decided rather than inherited.
- **Each game's plugin set** with the login module: the Login-button swap, the character-screen capacity with search and filter, the server address from the launch. EverQuest's is new, on the loader pattern of the DDO tools and thorium; claudelink stays a MacroQuest research tool.
- **Ankh Play for museum games.** Start the installed client aimed at the chosen server; the Characters tab lists the account's characters and offers launch-to-character.
- **Removal, per server at its release:** `accountDomain` with its pages, forms and API, the account pages, the claim flow, Ankh's Account list and `GET /me/gameAccounts`. The `gameAccount` tables and Ankh's account domain fields go once no server needs them.

Independent of these: multiboxing profiles and the Sessions board (DDO first), window layouts and per-instance settings files in the shared core.

## Decisions taken

- Login requires the module; no credential sync to emulators; no plain-client login.
- The client renders the character list the server sends; the module only extends capacity, sorting and filtering.
- One emulator account per player per server; the emulators are changed to allow many sessions and many characters per account.
- A credential lives for a short window: one password shared by the logins that ask within a minute of each other, scrambled after.
- Every modified server has an organization copy first.

## Open

- Where the EQEmu login work sits in the branch order (`rof2`, `guids`, `housing`, bots); the owner decides when EverQuest's turn comes.
- Whether the module presses Enter World itself after a launch-to-character pick, or leaves the player on the select screen.
- Per-IP session caps on each server (EQEmu `MaxClientsPerIP`, ACE `MaximumAllowedSessionsPerIPAddress`) for households with many clients.
