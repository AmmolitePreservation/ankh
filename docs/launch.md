# Launching games

A museum game's Play starts the installed client; an attached game (see `install.md`, Attached games) starts through its own launcher service or through Steam.

## A museum game

The manifest's world names the file name of the game's program (`executable`). Play finds the one installed file with that name, whatever folder the client package put it in, and starts it from its own folder; none or more than one stops with the reason, and a world without an `executable` has no Play yet. The client is aimed at its server by its client package.

A world with `givesCredential` is one whose client signs the player in: its login plugin asks the site for the player's own account in that world at each login, so the player types and chooses nothing. The plugin also takes the player's Ammolite name and password typed at the game's login screen, in a game Ankh started or not, and asks the site with them instead (a `POST` to the same address, which the site checks as a sign-in); a player returned to the login screen types them again. Play hands the client what that takes in its environment, `ankhPluginsCredentialURL` (the world's `/api/v1/servers/<id>/credential`) and `ankhPluginsCredentialToken` (the player's Ammolite token), which the plugin removes from the environment as it reads them. For such a world the game page shows no Account list and no Manage accounts. Every other museum game keeps its account list and starts at its own login screen.

## Logins kept on this computer

An attached game's accounts are the player's own and never reach Ammolite. **Manage accounts** on the game page opens the logins dialog: a list of the saved account names, a row to add one or replace a password, and Remove. Each login is a Windows Credential Manager generic credential for the current Windows user, target `Ankh/AmmolitePreservation/gameLogin/<game>/<variant>/<account>` (the parts percent-escaped), with the account name as the credential's user name and the password as its secret. The password is typed once and never shown again; any program running as that Windows user can read Credential Manager, which is the same standing as the Ammolite token.

The Account list on the game page holds those names and the Realm list holds the game's own worlds. Both choices are remembered per game in the settings (`attached/<id>/world`, `attached/<id>/account`).

## Through Steam

An attached edition whose launcher kind is `steam` is a game the player owns on Steam. It has no worlds and no logins of its own: the two lists on the game page say "Starts through Steam" and "Your Steam account", Manage accounts is off, and Play needs nothing chosen. Play opens `steam://rungameid/<steamAppID>`, as Steam's own shortcuts do, so Steam starts the game with the player signed in and the plugins in its folder load with it. Ankh does not learn whether the game started; it says that Steam was asked.

## The Turbine launch

An attached edition names its launcher kind in the manifest. The kind `turbineGLS` is what the Turbine and Standing Stone launchers do, read from the game folder's `ddo.launcherconfig` (or the like):

1. `Launcher.DataCenterService.GLS` and `DataCenter.GameName` come from that file. Every published `http://` address is used over `https://`.
2. `GetDatacenters` (SOAP) on the GLS service names the game's auth server, its server-side launcher configuration and its worlds, each with a chat server and a status address. The Realm list is these worlds in the service's order, fetched when the game page opens and kept for the session.
3. Play reads the server-side configuration (`GameClient.WIN64.Filename`, the `GameClient.WIN32.ArgTemplate`, the `GameClient.Arg.*` values, the login queue address and its parameter template), then calls `LoginAccount` with the saved login for a ticket; the account must hold exactly one subscription to the game. A wrong password is the service's own fault message on the status line.
4. The world's status document gives its login servers and queue addresses; a full or unavailable world stops here with the reason. A number is taken in the login queue the way the launcher does, and a refusing queue is reported with its HRESULT.
5. The template becomes the client's command line, with the subscription, login server, ticket, chat server and language filled in, and the client named by the server configuration is started from the game folder; it must be the edition's executable. The ticket lives only in those arguments, which the client scrubs once read.

The status line shows each step while it runs and holds Play. Nothing is kept from a launch; the next Play repeats every step. Ankh does not watch the started client.
