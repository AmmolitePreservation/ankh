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

- `ankh`: the latest Ankh release, read from GitHub by Ammolite (see `updates.md`). It is answered without a token too, so a signed-out Ankh still learns about updates. When Ammolite could not read GitHub it carries the reason instead.
- `worlds`, with a token: the worlds on display. Each becomes a library entry: the game's name, then the edition (for example Classic (1.12.1)). The world's name is not shown in the list but the search finds it, and the game page shows it as the Realm. A world's color is the tint of its icon, and its status line comes from the world itself and refreshes with every manifest.
- `packages`, with a token: every package attached to an edition, with its role (client, required tool, optional tool), its editions and its newest version: file name, size, SHA-256, the number of files inside and their size once extracted, and where to download it. The game page lists the packages of the selected world's edition by role.

`GET /api/v1/me/gameAccounts` supplies the logins the user has in the world's account domain, offered in the Account list.

A reply that is missing a field or has the wrong type is refused by name; nothing is guessed. Installing, launching and managing game accounts are not built yet.
