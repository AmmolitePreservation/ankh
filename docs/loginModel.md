# Login model: one account per server, the login module, and the emulator changes

A plan for how a player gets into a museum game, replacing game accounts and account domains. It is a plan, not a description of what exists; `install.md`, `launch.md` and `ammolite.md` say what exists.

## The problem

Every emulator we run (EQEmu, cmangos, ACE, Rusty Motors, Ourodev) lets one client log in per account, caps the characters an account may hold at what its client's select screen can show (RoF2 8, WoW 1.12 10, Asheron's Call 11, City of Heroes 12), and keeps characters per account. A player who wants many characters online at once therefore needs many accounts, each with a name and a password, and Ammolite today presents them as "game accounts" under "account domains"; one player on Peridot has 72. Emulator password storage is weak (cmangos keeps `SHA1(USER:PASS)`, EQEmu an unsalted hash), so an emulator account carries a random password of its own, never the player's Ammolite password.

## The model

- **One password, Ammolite's.** A player has one name and one password, their Ammolite ones. No game server stores a password a person chose.
- **One account per player per server.** The emulator account is the player's identity on that server, named after their Ammolite name, created and owned by Ammolite. All of the player's characters on that server live in it. Access is granted or removed per Ammolite user.
- **Each login server checks Ammolite.** Every game keeps its own client and its own servers; its login server asks Ammolite whether a name and password are good, instead of its own account table: EverQuest's login server, Rusty Motors (Motor City Online, behind clearcoat's secure login), ACE, City of Heroes' auth server. World of Warcraft is the exception: its client turns the password into an SRP value before realmd sees it, a weak one (`SHA1(USER:PASS)`), and SRP needs the server's value fixed before the proof. Its account keeps a random password that Ammolite sets, keeps and never shows, and a player reaches it only through the login module, which Ammolite answers after checking the player.
- **The server stays the source of the character list.** The client logs in, receives its characters from the server as it always did, renders them, and the player picks one. Nothing of ours draws a list.
- **The login module** is a required plugin of every museum game. It does three things and no more:
  1. When the player presses the game's own Login button with nothing typed in a game Ankh started, it asks Ammolite with Ankh's token; with a name and password typed, they are the player's Ammolite name and password, checked by Ammolite as a sign-in. Where the game's login server checks Ammolite itself, the module only widens what the client's boxes take; for World of Warcraft it obtains the account's password from Ammolite and completes the client's login with it.
  2. It lifts the client's character-screen capacity to an arbitrary count and adds sorting, search and a filter over the list the server sent.
  3. It carries the server address from the launch (argument or launch file) so one install serves several servers without editing `eqhost.txt` or `realmlist.wtf`.
- **The servers allow it.** Each emulator is changed, in our own fork, to hold any number of characters per account and to accept any number of sessions on one account at once, with exclusivity moved to the character (a character can be in the world once). Everything a server keys by account rather than character is reviewed: WoW's account bank and mail, EverQuest's shared bank, bans, account-wide flags and online counts stay account-wide, which is now what the player wants.
- **Servers own their accounts.** The server's handler configuration carries the account operations (`createAccount`, `changePassword`, `listCharacters`, and a new `moveCharacter`); account domains and login servers disappear as concepts.
- **Attached games** (Dungeons & Dragons Online) stay outside this model: the player's own accounts saved on the PC (`launch.md`), the game's own login screen, no server changes.

## What the player sees

- Sign in to Ankh once.
- Install a game and press Play. The client starts at its own login screen, aimed at the chosen server. Press Login; nothing to type.
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
- **Ammolite, credentials.** For World of Warcraft, Ammolite sets a random password through the handler when it creates the account and keeps it encrypted. `GET /api/v1/servers/<id>/credential` returns it to the signed-in player whose account it is, and `POST` on the same address to a name and password typed at the game's login screen, checked and locked out as a sign-in. Ammolite replaces it only when the player's access is removed or a reset is asked for. The emulator checks it as it checks any password.
- **Ammolite, the login servers' check.** A check for login servers only: a name and a password in, yes or no out, rate-limited, the lockout shared with the site's sign-in.
- **Each login server, in the organization's copy:** its password check asks Ammolite. EverQuest's login server no longer creates an account at an unknown name's first login (`account.auto_create_accounts` off; Ammolite creates the account).
- **Each emulator, in the organization's copy:** characters per account unlimited; several sessions per account with the lock per character; every rule the server keys by account decided rather than inherited.
- **Each game's plugin set** with the login module: the Login-button swap, login boxes that take an Ammolite name and password, the character-screen capacity with search and filter, the server address from the launch. EverQuest's is new, on the plugin base the other games use; claudelink stays for MacroQuest research.
- **Ankh Play for museum games.** Start the installed client aimed at the chosen server; the Characters tab lists the account's characters and offers launch-to-character.
- **Removal, per server at its release:** `accountDomain` with its pages, forms and API, the account pages, the claim flow, Ankh's Account list and `GET /me/gameAccounts`. The `gameAccount` tables and Ankh's account domain fields go once no server needs them.

Independent of these: multiboxing profiles and the Sessions board (DDO first), window layouts and per-instance settings files in the shared core.

## Decisions taken

- Ammolite's password is the only one a player has; each game's login server checks it with Ammolite and no emulator stores it. World of Warcraft keeps a random account password Ammolite sets, reached only through the login module, which is required there.
- A player can type their Ammolite name and password at the game's own login screen, with or without Ankh; the login module is needed where the stock client cannot take them.
- The client renders the character list the server sends; the module only extends capacity, sorting and filtering.
- One emulator account per player per server; the emulators are changed to allow many sessions and many characters per account.
- World of Warcraft's account password is fixed: set once by Ammolite and replaced only when access is removed or a reset is asked for.
- Every modified server has an organization copy first.

## Open

- Where the EQEmu login work sits in the branch order (`rof2`, `guids`, `housing`, bots); the owner decides when EverQuest's turn comes.
- Whether the module presses Enter World itself after a launch-to-character pick, or leaves the player on the select screen.
- Per-IP session caps on each server (EQEmu `MaxClientsPerIP`, ACE `MaximumAllowedSessionsPerIPAddress`) for households with many clients.
