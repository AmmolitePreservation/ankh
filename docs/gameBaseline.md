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
- **A3 Many at once.** Several sessions on one account with the lock on the character. Before a game is signed off, its largest event plus two groups is proven logged in at once from one account, once: World of Warcraft 50 (40 raid + 2 groups of 5), EverQuest 84 (72 raid + 2 groups of 6), Motor City Online 18 personas (a race of 6 + 2 races), City of Heroes 64 (a league of 48 + 2 teams of 8), Asheron's Call to be set by the owner. After that one test, a single party logged in at once from one account is the check.
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
| L1 | done; 128-character password not tested | partial; 128-character password not measured | partial: 128-character password typed (2026-11-23); a 32-character name not tried | done; typed 32-character name not tested | owner |
| L2 | done | done | done | done (Ankh's Play could not start it until 2026-11-23: the site gave no program) | owner |
| L3 | done: one-time keys realmd takes from its login_key table under NAME#ID | done | done | done | owner |
| L4 | done | done | done: the login dialog has no remember box; the game has no later login screen, a later login in the session asks for a restart | done | owner |
| L5 | done | done | done | done: empty boxes get a sentence in the game's failed-login dialog | partial: fatal errors in a Windows box |
| L6 | done: the client logs in only with keys Ammolite gives; the owner's own accounts keep their passwords until their move | done | done | done | owner |
| A1 | done | done | done | done | owner |
| A2 | done | done | done | done | owner |
| A3 | done (50 at once, 2026-11-13) | done (84 at once, 2026-11-13) | partial: lock on the persona (2026-11-23); 18 at once proven before it and the launch grants, so the at-scale test again on the owner's go | partial: the 64 at once is owed (4 proven) | owner (target not set) |
| A4 | done | done | done | done (never needed) | owner |
| A5 | done | done | done | done | owner |
| C1 | done | done | done | done | owner |
| C2 | done | done | done | done | owner |
| D1 | done | done | done | done | partial: display-guard optional |
| D2 | partial: world at 21:9 not measured | partial: ultrawide cursor not tested | partial: not tested | not tested | partial: not tested |
| D3 | done | not tested | not tested | done: no Ultra Mode prompt, no crash-report box | not tested |
| D4 | done | done | done | done | partial: no background rate |
| S1 | done | done | not checked | n/a (never shown) | not checked |
| S2 | done | done | done | done | partial: scarab's own server setting |
| S3 | done | done | done | done | done |

## Exceptions

None. (World of Warcraft's reusable account password was an exception until the owner's decision of 2026-11-22 that every game logs in the same way; it was closed on 2026-11-23 with one-time keys.)

## Work, in order

1. **Tests owed:** 128-character passwords and 32-character names typed in every game (Motor City Online's password done); the at-scale tests of Motor City Online (18) and City of Heroes (64) on the owner's go; D3 checks; D2 only when the owner asks for an ultrawide test.
2. **Asheron's Call:** the owner's talk on its login design first, then every row.
