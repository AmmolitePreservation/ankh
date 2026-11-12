# Ammolite backend

Ankh gets its catalog, accounts and downloads from the Ammolite site's JSON API (`/api/v1`, documented in the ammolite repository). The pages of the site and this client call the same service functions, so a rule changed there changes both.

## Address

`https://ammolite.unseencolor.com:1996`. The `ammolite/url` setting in `%APPDATA%\AmmolitePreservation\Ankh.ini` replaces it for testing against another instance. It must be a bare origin over https; plain http is accepted only for `localhost` and `127.0.0.1`. Anything else stops the application at start.

## Signing in

Sign in and account creation are a modal over the current page, so the user stays where they were. It opens on its own when there is no stored sign-in at start and when a sign-in ends while the application is open; the library and the Account section of Settings also open it.

- Sign in: `POST /api/v1/auth/token` with the username, password and the label `Ankh on <computer name>`. The answer carries a token, shown once.
- Create an account: `POST /api/v1/register` with an invite code, username, password and its confirmation, then the same sign-in. If the account is created but signing in fails, the message says so.
- The token is kept in Windows Credential Manager under `Ankh/AmmolitePreservation/apiToken` (generic credential, this Windows user, this computer). The password is never stored.
- Every other request sends `Authorization: Bearer <token>`. A 401 on any of them ends the session: the stored token is removed and the sign-in modal opens with the reason.
- The name at the top right of the window is the signed-in Ammolite username, or "Sign in" when signed out. Its menu offers Sign in or Sign out, Settings and Attributions.
- Sign out (Settings, Account) revokes the token on the server and removes it from Credential Manager. If the server cannot be reached the stored token is removed anyway and the message says it could not be revoked.

## The manifest

One request, `GET /api/v1/manifest`, carries everything that changes on the server side. Ankh sends it at start, after signing in or out, every 15 minutes, and from Settings -> About Ankh, with the ETag of the last answer so an unchanged manifest costs a 304.

- `ankh`: the newest Ankh release with the files that install it, and `releases`, the notes of each recent release newest first, read from GitHub by Ammolite (see `updates.md`). It is answered without a token too, so a signed-out Ankh still learns about updates. When Ammolite could not read GitHub it carries the reason instead.
- `worlds`, with a token: the worlds on display. Each becomes a library entry: the game's name, then the edition (for example Classic (1.12.1)). The world's name is not shown in the list but the search finds it, and the game page shows it as the Realm. A world's color is the tint of its icon, and its status line comes from the world itself and refreshes with every manifest.
- `packages`, with a token: every package attached to an edition, with its role (client, required plugin, optional plugin), a plugin's category (Stability, Display, Login, Portability, Automation or Research; empty for a client), its editions and its newest version: file name, size, SHA-256, the number of files inside and their size once extracted, and where to download it; a description and an optional screenshot (URL, SHA-256, type) that the Plugins tab shows, the image being `GET /api/v1/packages/<name>/screenshot` fetched with the bearer token. The game page lists the packages of the selected world's edition by role.
- Ankh reads the release notice (`ankh`) first and on its own. A collection it cannot read (a field missing, a role or category it does not know) is reported in the library with the reason and leaves the update offer working, so a release that reads the server's newer words can always be reached. Ammolite, for its part, answers an Ankh older than 2026.11.12.0013 (by its `User-Agent`) with the package roles that release reads, `requiredTool` and `optionalTool`.

- `attachedGames`, with a token: games Ammolite does not host, each with its game, variant, edition label, color, the executable that identifies a copy of it (see `install.md`, Attached games), and how it launches: `launcher` is `turbineGLS`, with `launcherConfig`, the launcher's configuration file in the game folder, or `steam`, with `steamAppID`, the game's Steam app ID (see `launch.md`); anything else refuses the manifest, which is why the site leaves a kind out for an Ankh released before it. The executable must be a relative path inside the game folder, the configuration a bare file name and the app ID a positive number.

`GET /api/v1/me/gameAccounts` supplies the logins the user has in the world's account domain, offered in the Account list.

`GET /api/v1/packages/<name>/versions/<version>/files` lists the files inside a package version (path, size, CRC-32, and whether the package marks it write-once); an install records them in its receipt and an update diffs them (see `install.md`). Downloads follow the manifest's `url` for each version with the same bearer token.

A reply that is missing a field or has the wrong type is refused by name; nothing is guessed. Launching and managing game accounts are not built yet.
