# Login model: one account per server, the login module, and the emulator changes

A plan for how a player gets into a museum game, replacing game accounts and account domains. It is a plan, not a description of what exists; `install.md`, `launch.md` and `ammolite.md` say what exists, and `gameBaseline.md` lists what every game must have and where each stands.

## The problem

Every emulator we run (EQEmu, cmangos, ACE, Rusty Motors, Ourodev) lets one client log in per account, caps the characters an account may hold at what its client's select screen can show (RoF2 8, WoW 1.12 10, Asheron's Call 11, City of Heroes 12), and keeps characters per account. A player who wants many characters online at once therefore needs many accounts, each with a name and a password, and Ammolite today presents them as "game accounts" under "account domains"; one player on Peridot has 72. Emulator password storage is weak (cmangos keeps `SHA1(USER:PASS)`, EQEmu an unsalted hash), so an emulator account carries a random password of its own, never the player's Ammolite password.

## The model

- **One password, Ammolite's.** A player has one name and one password, their Ammolite ones. No game server stores a password a person chose.
- **One account per player per server.** The emulator account is the player's identity on that server, named after their Ammolite name, created and owned by Ammolite. All of the player's characters on that server live in it. Access is granted or removed per Ammolite user.
- **Each login server checks Ammolite.** Every game keeps its own client and its own servers; its login server asks Ammolite whether a name and password, or a one-time key, are good, instead of its own account table: EverQuest's login server, Rusty Motors (Motor City Online, behind clearcoat's secure login), ACE, City of Heroes' auth server, and World of Warcraft's realmd. World of Warcraft's client turns the password into an SRP value before realmd sees it, and SRP needs the server's value fixed before the proof, so each client names the key it holds and realmd asks Ammolite for that key's value.
- **Nothing reusable in the client.** Play gives the game a launch grant for its world, good only for that world's credential and for ten minutes, never Ankh's own sign-in. The login module trades it for one-time keys for the first login (Motor City Online's launcher and game each log in once) and then forgets it; back at the login screen later, the player types.
- **The server stays the source of the character list.** The client logs in, receives its characters from the server as it always did, renders them, and the player picks one. Nothing of ours draws a list.
- **The login module** is a required plugin of every museum game. It does three things and no more:
  1. In a game Ankh started, it logs in once by itself with the launch grant, which Ammolite turns into a one-time key; with a name and password typed, they are the player's Ammolite name and password, which the game's login server checks with Ammolite. It widens what the client's boxes take where they are too short.
  2. It lifts the client's character-screen capacity to an arbitrary count and adds sorting, search and a filter over the list the server sent.
  3. It carries the server address from the launch (argument or launch file) so one install serves several servers without editing `eqhost.txt` or `realmlist.wtf`.
- **The servers allow it.** Each emulator is changed, in our own fork, to hold any number of characters per account and to accept any number of sessions on one account at once, with exclusivity moved to the character (a character can be in the world once). Everything a server keys by account rather than character is reviewed: WoW's account bank and mail, EverQuest's shared bank, bans, account-wide flags and online counts stay account-wide, which is now what the player wants.
- **Servers own their accounts.** The server's handler configuration carries the account operations (`createAccount`, `changePassword`, `listCharacters`, and a new `moveCharacter`); account domains and login servers disappear as concepts.
- **Attached games** (Dungeons & Dragons Online) stay outside this model: the player's own accounts saved on the PC (`launch.md`), the game's own login screen, no server changes.

## What the player sees

- Sign in to Ankh once.
- Install a game and press Play. The client logs in by itself and opens at the character screen of the chosen server.
- Or, with or without Ankh, type the Ammolite name and password at the game's own login screen. A stock client does this where its boxes take an Ammolite name (up to 32 characters) and password (up to 128) and its login server checks Ammolite; any other client needs the login module installed. A player returned to the login screen, by a disconnect for one, types them again.
- The game's own character screen shows every character, searchable and sortable. Pick one, play.
- Press Play again for another client and another character, as many as the machine carries.
- Launch straight to a character from the Characters tab or a profile, for missions and multiboxing: the same path with the pick made in advance.
- Never: a game account name, a game password, a domain, a claim, or a choice of account.

## Before any server change

The AmmolitePreservation organization holds a copy of every server we modify: a public fork for an upstream on GitHub, a private repository for any other origin. Today: EQEmu, mangos-classic, acemulator and rustymotors are public forks; ourodev-source is a private copy. Work on a server stops until its copy exists.

## Migration

`roadmap.md` holds the order: World of Warcraft Classic first, EverQuest last, and where each game's server changes are proven. The pieces:

- **Ammolite, one account per server.** The account verbs move onto the server handler and the server record keeps the connection configuration. Each handler gains `moveCharacter` (cmangos `characters.account`, EQEmu `character_data.account_id`, and the equivalents), and a merge moves every character of a player's old game accounts into one account per server, reversible by the same verb. Peridot's 72 become one when EverQuest's turn comes.
- **Ammolite, credentials.** A launch grant for one world, good for ten minutes and only for that world's credential, made at Play for the signed-in player (`POST /api/v1/servers/<id>/launchGrant`); traded for one-time keys the world's login server checks with Ammolite. Until every game has moved, `GET /api/v1/servers/<id>/credential` keeps answering Ankh releases that pass their own sign-in.
- **Ammolite, the login servers' check.** A check for login servers only: a name and a password in, yes or no out, rate-limited, the lockout shared with the site's sign-in.
- **Each login server, in the organization's copy:** its password check asks Ammolite. EverQuest's login server no longer creates an account at an unknown name's first login (`account.auto_create_accounts` off; Ammolite creates the account).
- **Each emulator, in the organization's copy:** characters per account unlimited; several sessions per account with the lock per character; every rule the server keys by account decided rather than inherited.
- **Each game's plugin set** with the login module: the Login-button swap, login boxes that take an Ammolite name and password, the character-screen capacity with search and filter, the server address from the launch. EverQuest's is new, on the plugin base the other games use; claudelink stays for MacroQuest research.
- **Ankh Play for museum games.** Start the installed client aimed at the chosen server; the Characters tab lists the account's characters and offers launch-to-character.
- **Removal, for every server at once (the owner's call: the site was too cumbersome to keep them while games migrate):** players see no game accounts anywhere. The site's login pages, claim flow and game account API are gone, `GET /me/gameAccounts` answers no domains for Ankh releases that still ask, and Ankh shows no Account list for a museum game. A world's login system becomes a setting of the world in the admin pages. The `gameAccount` table stays only for admin merges of logins claimed earlier, and goes once no server needs one.

Independent of these: multiboxing profiles and the Sessions board (DDO first), window layouts and per-instance settings files in the shared core.

## Decisions taken

- Ammolite's password is the only one a player has; each game's login server checks it with Ammolite and no emulator stores it. Every game logs in the same way, World of Warcraft included (owner, 2026-11-22).
- Play gives the game a short-lived launch grant for one world, never Ankh's sign-in; logins use one-time keys; a player returned to the login screen types.
- A player can type their Ammolite name and password at the game's own login screen, with or without Ankh; the login module is needed where the stock client cannot take them.
- The client renders the character list the server sends; the module only extends capacity, sorting and filtering.
- One emulator account per player per server; the emulators are changed to allow many sessions and many characters per account.
- Every modified server has an organization copy first.

## Open

- Where the EQEmu login work sits in the branch order (`rof2`, `guids`, `housing`, bots); the owner decides when EverQuest's turn comes.
- Whether the module presses Enter World itself after a launch-to-character pick, or leaves the player on the select screen.
- Per-IP session caps on each server (EQEmu `MaxClientsPerIP`, ACE `MaximumAllowedSessionsPerIPAddress`) for households with many clients.
