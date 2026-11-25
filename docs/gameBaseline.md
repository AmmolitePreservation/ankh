# Game baseline

What every Ammolite game has, server and client, and where each game stands. A game is done only when every row is done for it, or when the owner has accepted an exception for that row, written in the exceptions list below. A new game starts from this list; a decision the owner takes for every game becomes a row here. `loginModel.md` is the design behind the login and account rows, `roadmap.md` the order of work.

## Rows

### Login

- **L1 Typed login.** A player types their Ammolite name (3 to 32 letters and digits) and password (up to 128 characters) at the game's own login screen and gets in, with or without Ankh.
- **L2 Play logs in.** Ankh's Play takes the player to the character screen with nothing typed or pressed.
- **L3 Nothing reusable in the client.** Play gives the game a one-time launch grant for that world, good for a few minutes, never Ankh's own sign-in; each login uses a one-time key the site issues for it. No password or token that could log in again stays in the game's environment, memory or files.
- **L4 Retyping after the first login.** Back at the login screen later in a session (a disconnect, logging out), the player types their name and password.
- **L5 Refusals in the game.** A refused or failed login is shown in the game's own dialog, never a Windows message box.
- **L6 The login server asks Ammolite.** The game's login server checks names, passwords and keys with Ammolite; no emulator stores a password a person chose.

### Accounts

- **A1 One account per player per server**, named after the Ammolite username, made by Ammolite.
- **A2 At least 100 characters per account.**
- **A3 Many at once.** Several sessions on one account with the lock on the character. Before a game is signed off, its largest event plus two groups is proven logged in at once from one account, once: World of Warcraft 50 (40 raid + 2 groups of 5), EverQuest 84 (72 raid + 2 groups of 6), Motor City Online 18 personas (a race of 6 + 2 races), City of Heroes 64 (a league of 48 + 2 teams of 8), Asheron's Call 54 (18 in hard content and 36 farming beside them). After that one test, a single party logged in at once from one account is the check.
- **A4 Merging.** A player's old accounts on a server merge into theirs, and the merge can be undone.
- **A5 No account screens.** Ankh shows no account list or choice for the game.

### Character screen

- **C1 Every character.** The game's character screen holds and scrolls every character of the account.
- **C2 Search and filter.** A name search that filters as you type, and a game-specific filter the owner approved before it was built (Motor City Online: search only).

### Display

- **D1 Borderless.** The game runs borderless at the monitor's own mode, never exclusive fullscreen.
- **D2 Any shape.** The picture keeps its shape and the cursor lands where it is drawn at any aspect ratio, ultrawide included.
- **D3 No flashes or popups.** Nothing flashes over the game or pops up beside it in transitions and at start (ghost windows, first-run dialogs, crash notices).
- **D4 Background pacing.** frame-control ships with the game, so clients in the background can be paced.

### Startup and stability

- **S1 License screens once.** Terms and license screens are accepted once, not at every start.
- **S2 Server from the launch.** The game connects to the server Ankh's launch names.
- **S3 Reports without hooks.** stability-guard reports crashes and freezes; nothing of ours installs a system-wide input hook.

## Status

done, partial, missing, n/a; "owner" where the owner's decision comes first. From the audit of 2026-11-22 against the code and recorded tests.

| Row | World of Warcraft | EverQuest | Motor City Online | City of Heroes | Asheron's Call |
|---|---|---|---|---|---|
| L1 | done (32-letter name and 128-character password typed 2026-11-23) | done (both filled through the game's own text setter 2026-11-23; the login screen's boxes set no length) | done (both typed 2026-11-23) | done (both typed 2026-11-23) | partial: the in-game sign-in (server list, name and password, saved logins in Ankh's store) released 2026-11-25; a 32-letter name and long password not yet typed |
| L2 | done (through Ankh 2026-11-23; intro-skip since 2026-11-23: the intro at a first start plays until a click or key, and skipWholeIntro skips it) | done (through Ankh 2026-11-23, a fresh install's seizure warning, news and server choice included) | done (through Ankh 2026-11-23; the intro plays until a click, the owner's choice, and intro-skip's skipWholeIntro skips it) | done (Ankh's Play could not start it until 2026-11-23: the site gave no program) | done for Pyreal (Play to the character screen, 2026-11-25) |
| L3 | done: one-time keys realmd takes from its login_key table under NAME#ID | done | done | done | done: Pyreal's ACE checks the one-time key with Ammolite |
| L4 | done | done | done: the login dialog has no remember box; the game has no later login screen, a later login in the session asks for a restart | done | owner |
| L5 | done | done | done | done: empty boxes get a sentence in the game's failed-login dialog | partial: fatal errors in a Windows box |
| L6 | done: the client logs in only with keys Ammolite gives; the owner's own accounts keep their passwords until their move | done | done | done | done: Pyreal's ACE checks every password login with Ammolite (2026-11-25) |
| A1 | done | done | done | done | done: made by Ammolite at the first good login |
| A2 | done | done | done | done | partial: Pyreal allows 100; the client's character screen shows 20 |
| A3 | done (50 at once, 2026-11-13) | done (84 at once, 2026-11-13) | done (18 at once, each its own persona, with the lock on the persona, 2026-11-23) | partial: the 64 at once is owed (4 proven) | partial: two sessions on one account proven; the 54 at once is owed |
| A4 | done | done | done | done (never needed) | owner |
| A5 | done | done | done | done | done |
| C1 | done | done | done | done | owner |
| C2 | done | done | done | done | owner |
| D1 | done | done | done | done | partial: display-guard optional |
| D2 | partial: world at 21:9 not measured | partial: ultrawide cursor not tested | partial: not tested | not tested | partial: not tested |
| D3 | done | done: a fresh install's seizure warning and news pass by themselves | done: the false low-RAM box is gone (2026-11-23); a fresh install from the released packages opens no dialog (2026-11-24); the game's own startup picture shows for half a second before its window, kept by the owner's choice | done: no Ultra Mode prompt, no crash-report box | not tested |
| D4 | done | done | done | done | partial: no background rate |
| S1 | done | done | not checked | n/a (never shown) | not checked |
| S2 | done | done | done | done | done: Play passes Pyreal's address on the command line |
| S3 | done | done | done | done | done |

## Exceptions

None. (World of Warcraft's reusable account password was an exception until the owner's decision of 2026-11-22 that every game logs in the same way; it was closed on 2026-11-23 with one-time keys.)

## Work, in order

1. **Tests owed:** City of Heroes' 64 at once (on the owner's go); D2 only when the owner asks for an ultrawide test.
2. **Asheron's Call:** Pyreal to the full baseline first (owner, 2026-11-25), with the old `scarab` plugin removed and rebuilt as Ankh-style plugins; community servers are not held to these rows: their accounts and passwords are the player's own, kept in Ankh's login store, and Ankh's launch profiles start them.
