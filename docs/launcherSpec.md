# Private MMO Launcher — Product and Interface Specification

Version 0.3 · 30 October 2026 · Proposed application design

**Ankh** and **MMOnomicon** are two skins for the same application. They share information architecture, behavior, terminology, accessibility, and feature availability. This specification describes a desktop launcher for a private household collection of preserved games. It is a design proposal, not an implementation or an audit of Scarab.

The product starts with **download and play**. It can grow into a local workspace for client maintenance, characters, mixed-game sessions, window and input management, scripting, server-side character bots, and MCP access. A person who only wants to play one game never needs to configure the advanced workspace.

## 1. Product decisions

| Decision | Result in the application |
| --- | --- |
| Required fixes are part of a working installation. | Install includes the exact connection and compatibility components required by that game build and realm. They are visible under Client tools but are not optional toggles. |
| Optional features are independently enabled. | Client enhancements, Sessions, Automation, and MCP can each be enabled without enabling the others. Higher layers reuse the same underlying services. |
| A mixed-game session is a first-class object. | One saved session can contain 12 EverQuest clients, one World of Warcraft client, and one Motor City Online client. |
| Layouts and input groups are separate. | All 14 windows can share a layout while only the EQ clients belong to an EQ input group. |
| Server bots and client sessions are separate runtimes. | A character can run as a server-side bot without opening a game window. EQ and WoW bots share one management workspace while retaining their own worlds, groups, and rules. |
| Game integration has explicit capabilities. | Window management does not imply access to character state, gameplay commands, or automatic login. Unsupported features are explained. |
| One control plane serves every interface. | The GUI, scripts, and MCP call the same validated operations and report the same jobs, results, and errors. |
| The collection remains private and useful offline. | No public discovery, store, social feed, mandatory cloud account, or online activation. Network access is for configured household services and approved package sources. |
| Character viewing is useful without live game access. | Show a timestamped server or client snapshot when available, and a clear unavailable state otherwise. |
| Presentation is restrained. | Two main columns, compact rows, familiar controls, no hero image. Textures and small game icons carry the identity. |

### Assumptions

- First implementation targets Windows desktop because the collection and requested tooling center on legacy Windows clients. Other platforms are later adapter and packaging work.
- Initial orchestration runs on one PC. A household catalog or server can supply manifests and character data; controlling clients on other PCs is a later explicit extension.
- Household members have separate local launcher profiles. They can share downloaded game content without sharing account credentials or optional tool settings.
- The owner curates game releases, realms, required packages, and compatible optional tools. Ordinary players select from those releases.
- Existing tools can be integrated through adapters. ISBoxer and MacroQuest are reference models; the product does not assume their code, licenses, APIs, or compatibility can be reused unchanged.
- Names, realms, characters, package versions, status indicators, and integrations in mockups are illustrative. The screenshots do not establish that any adapter already exists.

## 2. Layers and feature discovery

Layers describe increasing capability, not a subscription ladder or a mandatory setup sequence.

| Layer | Included behavior | Entry point | Default |
| --- | --- | --- | --- |
| 1 · Play | Catalog, download, import, required setup, update, launch, basic accounts, media | Library → Play | On |
| 1a · Characters | Character list, equipment, location, last snapshot, launch context | Library → Characters | Appears where supported; live integration optional |
| 2 · Required compatibility | Build-specific dependencies, connection configuration, essential patches | Library → Client tools → Required | Included in Install; maintained automatically |
| 3 · Optional client tools | Borderless window support, tested stability bundles, renderer or input fixes, optional integrations | Library → Client tools → Optional | Off until chosen |
| 4 · Sessions | Mixed-game rosters, instance isolation, window presets, focus switching, scoped input groups | Sessions | Off until enabled |
| 5 · Automation | Shared object model, Lua scripts, events, game adapters, GUI controls | Automation | Off until enabled |
| 6 · Server bots | Activate characters on servers; track maps and activity; manage groups, missions, inventory, and trades | Bots | Off until enabled and connected to a capable server |
| 7 · MCP | External clients inspecting and controlling allowed launcher operations, including granted bot operations | Connections → MCP | Off until enabled |

**Settings → Features** has independent switches for Sessions, Automation, Bots, and MCP connections. Enabling one reveals its top navigation destination. Disabling it hides its UI after resolving active work. Server bots can continue on their servers if the user chooses; closing the UI is never an implicit stop command. It does not uninstall games. Installing a tool and enabling that tool for a particular launch profile are separate decisions.

Optional tools remain discoverable from a game's Client tools tab even when Sessions and Automation are hidden. A quiet “More ways to play” link in Settings explains the available features. Avoid recurring prompts on the Play page.

Dependencies are operational, not arbitrary: a script that arranges windows needs the window service, not a complete multiboxing setup; an MCP client can inspect the library without installing a game bridge or enabling Automation.

## 3. Primary tasks

1. A household member selects a game, presses **Install**, and then **Play**. Required fixes arrive with the install.
2. A player imports an existing client. The launcher identifies the build, explains any mismatch, and offers a managed copy or an explicit managed import.
3. A player enables a borderless tool for one launch profile without changing everyone else's client configuration.
4. A player views a character snapshot, selects the corresponding realm and account, and starts the game. Automatic character selection only appears when supported.
5. An advanced user saves a 14-client mixed-game session, places it across monitors, and launches it with a preflight check.
6. While that session is running, the user switches the main window, pauses an input group, and relaunches one failed slot without disturbing the rest.
7. A script reads a typed client list, reacts to an instance becoming ready, and applies a layout. A game-specific script can use richer data only on clients whose adapter exposes it.
8. An MCP client asks what is installed, previews a session launch, and starts an already configured session within a stored grant. Activity shows what it did.
9. The user activates owned characters as server-side bots, monitors EQ and WoW together, groups compatible bots, assigns bounded missions, and manages inventory and in-world trades.

## 4. Application shell

### Layout

- Reference desktop size: **1600 × 1000 logical pixels**. Comfortable minimum: **1180 × 760**. Respect system scaling rather than shrinking text.
- Native title bar: 32 px. Application header: 60–84 px. Left context pane: 360 px by default, resizable from 300 to 420 px. Main pane uses the remaining width.
- **Divider alignment is a shell invariant:** the wordmark area and the context pane have exactly the same width. Use one continuous vertical divider from the top edge through the header to the bottom edge. There is no separately sized brand box. Top navigation starts inside the main pane with a normal content gutter. Apply this to both Ankh and MMOnomicon, on every page.
- Define sidebar width once in the shared layout component and reuse it for the wordmark and context pane. Resizing or changing skins must update both together; do not calculate their widths independently.
- Exactly **two primary columns**: context list and selected detail. Top navigation is horizontal, not a third vertical rail. Editors may have internal panels or a temporary drawer.
- Header: small brand mark and wordmark at left; **Library / Sessions / Bots / Automation / Connections** across the top when enabled; compact household profile menu at right.
- Left pane changes with destination: game library, saved sessions, script list, or connection list. Settings and Activity remain at its foot.
- The main pane owns its scroll position. The context list scrolls independently. Selection, search, sort, filters, and scroll position survive navigation.
- A contextual bottom action bar stays within the main pane. It shows status and the primary action for that page; it never covers the final row of content.
- Background jobs appear in a small global Activity indicator. Opening it produces a drawer with downloads, patching, preflight, scripts, and connection activity.

### Navigation

| Destination | Left pane | Main pane |
| --- | --- | --- |
| Library | Searchable, sortable game releases | Play / Characters / Client tools / Media / About |
| Sessions | Saved session presets, running sessions | Overview / Layout / Input / Runtime |
| Bots | All worlds, realm filters, and saved bot collections | Roster / Map / Missions / Inventory / Activity |
| Automation | Searchable scripts and GUI control panels | Editor / Controls / Runs / API browser |
| Connections | MCP and other enabled local integrations | Connection setup, grants, requests, activity |

The simple default exposes Library and Settings. Screens in this concept set deliberately show advanced navigation enabled so the larger product is visible.

### Library rows

- 64 px row height, 36–40 px game icon, title and edition on separate lines, favorite star, and a small status icon when attention is needed.
- Search covers title, edition, tags, and configured realm aliases. Filter: All / Installed / Favorites. Sort: Custom / Name / Recently played / Install state.
- Virtualize long lists; there is no fixed game limit in the UI. Filter counts, empty results, keyboard movement, and a visible scrollbar are required.
- A game release is selectable independently of other releases of the same game. WoW Classic, TBC, and WotLK are distinct entries.

Illustrative initial rows:

| Game | Edition | Small icon direction |
| --- | --- | --- |
| EverQuest | Classic | Original Qeynos gate, yellow banners |
| Asheron's Call | Classic | Original Dereth stone towers |
| World of Warcraft | Classic | Original Naxxramas, green plague light |
| World of Warcraft | The Burning Crusade | Dark Portal, red Hellfire sky |
| World of Warcraft | Wrath of the Lich King | Frostmourne or icy Icecrown silhouette |
| Warhammer Online | Age of Reckoning | Empire and Chaos heraldry, medieval rather than 40K |
| Motor City Online | Original | Red vintage coupe, chrome and period neon |

## 5. Library → Play

**Purpose:** get one client running with the fewest meaningful choices.

### Page organization

1. **Game header:** 48 px icon; game title; edition and build beneath; one Game options menu. No scenic banner.
2. **Tabs:** Play, Characters when supported, Client tools, Media, About.
3. **Launch setup:** Realm dropdown, Account dropdown, and a collapsed More launch settings disclosure. The last valid choices are remembered per person and release.
4. **Client readiness:** one status line and a concise maintenance summary. Example: “Up to date · Required compatibility ready.” An optional-tools summary is a small link, not a promotional card.
5. **Continue with:** at most one small recent-character row when reliable data exists. It identifies the account/realm context and does not imply automatic in-game selection.
6. **Bottom action:** status at left, primary Install / Update / Repair / Play action at right. When ready: “Ready to play” and “Arcanite · Main account.”

The default profile does not require users to understand tool bundles. More launch settings exposes launch profile, display mode, and additional arguments only if relevant. Manage accounts is a secondary link near Account.

### Primary action states

| State | Primary action | Supporting behavior |
| --- | --- | --- |
| Not installed | Install | Destination and total disk requirement appear in a compact sheet. Required packages are included. |
| Downloading | Pause | Bytes, speed, progress, and cancel in Activity; resume after interruption. |
| Verifying / applying | Progress state | Show the current step. Cancellation waits for a safe checkpoint or rolls back. |
| Required update | Update | Explain which release or required component changed. |
| Optional update only | Play | Small update notice in Client tools; do not block launch. |
| Required component broken | Repair | Explain the failing component and planned repair. |
| Ready | Play | Starts the selected single-client launch profile. |
| Running, additional instance supported | Launch another | Show “1 running”; focus existing instance is a secondary action. |
| Running, additional instance unsupported | Focus game | Explain the build's instance restriction in Game options. |
| Server unavailable | Play or disabled with explanation | Depends on whether this release supports offline or alternate realms. Never confuse an unavailable server with a broken installation. |

Do not offer Update while a process is using files that would change. Queue it for after exit or prepare a side-by-side release.

## 6. Library → Characters

**Purpose:** inspect the people and possessions associated with a game without opening a client.

### Organization

- Keep the selected game's header and tabs.
- Filter bar: Account, Realm, character search, Refresh.
- Compact character table: Name, Level, Race / Class or game-appropriate equivalent, Location, Last updated. Do not force level/class fields onto a racing game.
- Selecting a row opens an **inline detail area below the table**, preserving the two-column shell. Summary, Equipment, Inventory, and Progress tabs appear only when supplied by the adapter.
- Summary shows a small portrait or neutral silhouette, actual identity, source, timestamp, and status. No large generated character art presented as actual game data.
- Equipment is a labelled slot list or compact slot grid. Item tooltips come from the matching game build's data.
- Bottom action: **Play with this account**. Change it to **Launch this character** only when the game adapter can actually select that character.

### Data rules

Each field has a source and freshness timestamp. Source precedence is configured by the owner: authorized server API/export, supported client bridge, cached snapshot, or manual entry. An adapter can combine sources but must identify stale or missing values.

Offline snapshot: “Last synced 2 hours ago.” No connection: “Connect a character source” with setup instructions, or “Character viewing isn't available for this release.” Distinguish an empty account from a failed request. Refresh never starts a game or installs a bridge without explanation.

For MCO, a future adapter could show drivers and garages. For a game with no reliable source, omit the character tab rather than inventing data.

## 7. Library → Client tools

**Purpose:** make the distinction between essential maintenance and chosen enhancements obvious.

### Page organization

1. Build and launch-profile selector: “EverQuest · Classic” and “Profile: Default.”
2. **Required for this client:** compact checked rows. Each has name, purpose, version, and health. Examples: connection setup, legacy runtime, build compatibility patch. No off switches.
3. **Optional tools:** searchable rows filtered to this exact build. Each has name, short benefit, compatible version, state, and one action. Examples: Borderless window, Stability bundle, Background frame limit, Game bridge.
4. Selecting a row expands inline details: source, changed files/configuration, dependencies, conflicts, activation scope, restart requirement, and rollback availability.
5. Bottom action appears only for pending changes: “2 changes · No games will close” and **Apply changes**. Otherwise use “All changes applied”; avoid a redundant permanent save button.

### Tool lifecycle

| State | Row action |
| --- | --- |
| Available | Install |
| Installed, disabled for this profile | Enable |
| Enabled | Configure / Disable in row menu |
| Update available | Update, with version notes |
| Incompatible with selected build | Disabled action and a concrete reason |
| Conflict | Resolve before applying |
| In use, restart required | Apply on next launch |
| Broken | Repair / Restore previous version |

Installation stores a package once. Enablement selects it for a profile or instance. Required components belong to the release baseline. Optional tools use per-profile settings or private writable overlays; if a tool changes shared game files, the UI states the scope and creates a separate managed variant when necessary.

“Stability bundle” is a curated, build-specific collection with named contents. It never claims to prevent every crash. Tool actions show real compatibility metadata rather than universal promises.

### Package integrity and conflicts

- Manifests pin package identity, version, compatible game builds, checksums, dependency graph, touched files, configuration schema, and rollback strategy.
- Resolve the whole plan before modifying files. Conflicting writes cannot silently overwrite each other.
- Stage downloads, verify hashes, back up affected configuration, and commit atomically where possible. Keep a recoverable journal where atomic replacement is unavailable.
- Record the effective release and enabled tool versions in each launch and session run.
- A user can launch without optional tools to diagnose a problem while retaining required connection fixes.
- Package provenance is visible in details. Never infer a trusted package merely from its filename.

## 8. Sessions: mixed-game multiboxing

**Purpose:** launch and operate a deliberate set of clients as one workspace.

A session has a saved definition and one or more historical runs. Editing a preset does not silently alter a running session.

### Saved session definition

Each slot has a stable slot ID, display name, game release, realm, account reference, optional character reference, launch profile, tool selection, config overlay, layout role, and input-group memberships. Account secrets remain in the credential store.

The roster supports individual rows and repeatable groups. “EQ party × 12” expands to twelve independently addressable slots. Bulk editing can assign accounts and layout roles without hiding duplicate-account conflicts.

### Required example

Session: **Mixed worlds** — **14 clients · 3 games**.

| Slots | Release | Input membership | Initial layout role |
| --- | --- | --- | --- |
| EQ01–EQ12 | EverQuest · Classic | EQ party | EQ01 main; EQ02–EQ12 auxiliaries |
| WOW01 | World of Warcraft · Classic | None by default | Independent side window |
| MCO01 | Motor City Online · Original | None by default | Independent side window |

Illustrative two-monitor layout: Monitor 1 is 2560 × 1440 with a large 16:9 main region at left and two small 16:9 regions stacked at right. Monitor 2 is 2560 × 1440 with a 4 × 3 grid of 16:9 slots; eleven are assigned and one is empty. Total assigned regions: 3 + 11 = 14. EQ01 occupies the main region, WoW and MCO occupy the two side regions, and EQ02–EQ12 fill eleven grid regions. This is a window layout preview, not a collage of game artwork.

These are illustrative window rectangles. A real adapter may need minimum client dimensions, letterboxing, render-resolution scaling, or another preset. Validate the selected clients before claiming the preset is usable.

### Sessions GUI

- Left pane: saved presets with name, client count, and last-run status; New session at top.
- Main header: Mixed worlds, “14 clients · 3 games,” Save / Duplicate menu.
- Tabs: Overview, Layout, Input, Runtime.
- Overview: roster table and grouped launch settings. Add clients, duplicate slots, move rows, and per-slot validation.
- Layout: monitor canvas, named region tiles, preset selector, active-slot swapping, and Edit layout. A compact roster summary remains below the canvas.
- Input: named target groups, mapped actions, physical key bindings, repeat modes, and one visible master pause control.
- Runtime: per-slot process/window/integration status, logs, focus, restart, and stop. Distinguish process started, window found, login needed, connected, and in-world.
- Bottom action before launch: **Review & launch**. Preflight opens a review sheet and changes the final action to **Launch 14 clients**. A saved unchanged preset can use a configured quick-launch preference after successful validation.
- While running: **Focus main**, **Reapply layout**, **Pause input**, and a secondary Stop session menu. Layout, runtime, and input state stay visible.

### Preflight and launch order

Preflight resolves exact builds and tools; verifies accounts and unsupported duplicate logins; checks free disk and a conservative resource estimate; checks adapters, monitor topology, per-instance configuration, window mode, and slot minimum sizes. Estimates are labelled estimates, not guarantees.

The review sheet shows client counts by game, required repairs/downloads, warnings, input state, and launch order. A missing optional bridge does not prevent a window-only slot unless a required script depends on it.

Launch is staggered using adapter readiness signals, with configurable concurrency. It does not send credentials to the next process merely because a timer expired. On one-slot failure, retain successful clients, mark that slot, and offer retry or skip. Do not restart all fourteen clients.

### Window behavior

- Identify processes by session slot and process generation, then discover their relevant windows. Do not rely on window titles alone.
- Stable slot-to-region assignment survives crashes and process ID changes.
- Save monitor identity, normalized geometry, scaling, and role assignments. On missing monitors, offer a previewed fallback rather than placing windows offscreen.
- Swapping an active slot into the main region is explicit per swap group. WoW/MCO do not unexpectedly swap when interacting with an EQ group.
- Focus switching works without input broadcasting. Borderless, resizing, background frame limits, and audio selection are capability-dependent.
- Preview tiles are labels and optional live thumbnails. Avoid streaming fourteen high-rate previews by default.

### Input behavior

- One physical binding can target a named group or invoke a semantic action through the relevant adapters.
- Raw keyboard/mouse repetition and mapped game actions are separate features. Cross-game semantic mapping requires a mapping for each game; an EQ keybind is not assumed meaningful in WoW or MCO.
- Default input repetition is off. The example group targets EQ01–EQ12 only. WoW and MCO remain excluded unless explicitly added.
- Show the exact targets when enabled. Pausing input is global and immediate; a configurable emergency shortcut remains available outside the launcher.
- Exclude launcher forms and credential-entry windows from gameplay repetition. Pause on unexpected focus, an unrecognized window, or a client crash; never replay queued input into a replacement process.
- Script stop, input pause, and process stop are distinct commands. Stopping a script does not close games.

## 9. Automation: shared abstraction and scripting

**Purpose:** offer an MQ-style inspectable model across supported games while keeping game-specific richness.

### Model

The proposed common API exposes **Library, Games, Releases, Accounts (references only), Characters, Sessions, Slots, Instances, Windows, Tools, Jobs, and Events**. Each instance reports supported capabilities. Optional game namespaces expose richer objects: for example, EQ character/spawn/target information or a future MCO driver/garage model. Those namespaces are not part of a fake universal combat interface.

| Capability | Meaning | Can exist without a game bridge? |
| --- | --- | --- |
| process.lifecycle | Start, observe, and stop a managed client | Yes |
| window.manage | Locate, focus, and arrange windows | Yes, subject to client behavior |
| input.mapped | Send configured actions to explicit targets | Sometimes; adapter-dependent |
| character.snapshot | Read a cached or server-supplied character snapshot | Yes, if a data source exists |
| character.live | Read current in-game state | Requires a supported live integration |
| game.commands | Invoke game-specific commands | Requires a supported command adapter |
| game.events | Subscribe to game-specific state changes | Requires a supported event adapter |

An unsupported capability returns a typed error and appears unavailable in the API browser. A missing value is not zero, false, or a fabricated character state.

### Runtime and example

Lua is the proposed first scripting language because it suits the requested MQ-style workflow. This is a product choice, not a promise of source compatibility with existing MacroQuest scripts. An EQ adapter could expose an explicitly labelled compatibility bridge later.

Illustrative proposed API, not runnable code:

```lua
local session = launcher.sessions.get("mixed-worlds")
local eq = session:instances({ game = "everquest" })

launcher.ui.button("Arrange EQ", function()
  launcher.windows.apply_layout("eq-main-and-aux", { targets = eq })
end)

session:on("instance.ready", function(event)
  local instance = launcher.instances.get(event.instance_id)
  if instance:has("character.live") then
    launcher.log.info(instance.character:name())
  end
end)
```

Selectors resolve to explicit stable IDs. Mutating calls validate the target, process generation, grant, and capability at execution. Events include timestamps, source, instance ID, and schema version. Reads expose freshness; subscriptions support cancellation and backpressure.

### GUI organization

- Left: script search, New script, saved scripts with game scope and run state. Example scripts: Arrange on launch, EQ status panel, Recover missing window.
- Main header: script name, language, saved status; session selector.
- Tabs: Editor, Controls, Runs, API browser.
- Editor: real code editor with line numbers, syntax highlighting, diagnostics, Run / Stop, and a resizable lower output pane.
- Controls: script-declared buttons, checkboxes, selectors, status labels, and small tables rendered as native application controls. Scripts cannot replace the shell or add an unrestricted embedded browser.
- API browser: searchable typed objects, properties, events, capability requirements, and a target inspector. Show EQ bridge available and MCO window-only when that is the configured reality.
- Runs: duration, targets, granted capabilities, result, and logs. A run is pinned to a script revision and can be stopped independently.

### Execution boundaries

Run scripts outside the UI process. Provide CPU/time budgets, memory limits, cancellation, and a scoped API. No raw file, network, process, or native-library access in the normal script environment. Trusted native plugins are a separate package class with explicit provenance and enablement; a Lua checkbox cannot turn native code into a sandbox.

Grant script capabilities when enabling or changing a script, with a review of newly requested powers. Repeated use of an unchanged script within its grant should not produce repetitive confirmation dialogs. Start-on-session is an explicit per-script setting. Script failures surface in Automation and Activity without crashing clients or the launcher.

## 10. Connections → MCP

**Purpose:** let an external assistant use the same application operations that the GUI and scripts use.

MCP is an interface to the local control plane, not a separate automation engine. A library-only connection works without enabling scripts. A connection cannot obtain capabilities unavailable to the signed-in household profile or selected game adapter.

### Connection UX

- Left: Local MCP server, configured external clients, and connection status.
- Main header: **MCP connections**, global enabled switch, “Local only” scope.
- Setup card: transport, status, Copy client configuration, Test connection. Default to a client-launched stdio bridge connected to the local broker. Optional authenticated loopback HTTP is a later/configurable transport, not a public network listener.
- Clients table: client label, last seen, grant preset, status, Revoke.
- Permission groups: Library & characters; Install & maintain; Sessions & windows; Script execution; Game commands; Observe bots; Control bots & missions; Move items & trade. Each shows its allowed targets and capability limits.
- Recent activity table: time, client, operation, target, result. Long work links to Activity.
- Do not put a chatbot or a conversational hero panel on this screen. It is a connection and permission interface.

### Proposed API surface

These are application tool names, not changes to the MCP wire protocol.

| Tool family | Examples | Behavior |
| --- | --- | --- |
| Read | library.list, game.get, character.list, instance.inspect, job.get | Read within the client's grant; respect stale/cached markers. |
| Plan | game.plan_install, tools.plan_changes, session.preflight | Return a bounded preview, dependencies, targets, and warnings. |
| Maintain | game.install, game.repair, tools.apply_plan | Use a validated plan ID and return a job reference. |
| Sessions | session.list, session.launch, session.stop, windows.apply_layout | Explicit preset or slot IDs; do not silently broaden a target set. |
| Scripts | script.list, script.run, script.stop | Only enabled scripts and granted revisions/capabilities. |
| Game actions | instance.invoke | Namespaced adapter command, schema-checked arguments, explicit instance targets. |
| Bots | bots.list, bots.activate, missions.plan, missions.start | Server capability checks, explicit bot/world targets, separate bot-control grant. |
| Inventory | inventory.inspect, inventory.plan_move, trades.plan, trades.commit | Versioned server plans, item-specific authority, and authoritative receipts. |

Expose read-only resources for game manifests, capability descriptions, snapshots, and run summaries where useful. The app's durable job IDs and session-run IDs belong in the application model; they must not depend on a connection staying open.

### Grants and control

Provide presets **Inspect only**, **Manage launches**, and **Custom**. A grant records client identity/credential, household profile, allowed operations, target games/sessions, expiry, and whether an action needs review. A display name alone is not authentication. For stdio, pair the bridge to the broker with a locally protected per-client credential; for HTTP, use the selected MCP transport's current authorization requirements.

Read-only inspection and allowed actions on existing presets can run without repeated prompts. Installation/removal, new script powers, or ungranted game commands request a concrete plan review. The owner can change this policy deliberately. This policy is part of the proposed product, not a request for approval to create these mockups.

Revoking a grant rejects new calls and cancels cancellable pending work; it does not forcibly close running games. The user can choose Stop launched session separately. Never return passwords, unrestricted file contents, or raw credential references to an MCP client. Audit operation summaries with secret redaction.

## 11. Bots: server-side characters across worlds

**Purpose:** activate owned characters under server-side AI, see what they are doing, direct their work, and manage their possessions without running a desktop game client for every character.

This is a separate execution mode from Sessions. A client instance is a local process/window. A bot is a server-owned runtime associated with a character or an explicitly distinct companion entity. One can exist without the other. The launcher is a control and observation surface; the server executes movement, combat, crafting, inventory, and game rules.

### Character identity and handoff

The preferred model is **the user's actual character running under bot control**, not an unrelated NPC with the same display name. A server adapter must declare whether it supports existing player characters, separate companion bots, or both. Preserve those identities in the UI; never present a copied companion as the original character.

An execution lease on the server prevents the same character from being controlled simultaneously by a player client, two bot controllers, or two incompatible missions. Activation atomically checks ownership, realm eligibility, server limits, and the current lease. “Play this character” offers an explicit handoff: pause its mission, reach a server-defined safe checkpoint, persist state, release bot control, and let the client authenticate. It must not clone the inventory or force a second login. Reverse handoff uses the same authoritative character record.

An adapter that only supports companion bots still appears in Bots, labelled **Companion**, with its own capabilities. Existing EQEmu bot commands are not evidence that arbitrary existing EQ player characters can be converted into autonomous farmers. That would require additional server work.

### Cross-game organization

- The **Bots** destination combines all configured supported worlds in one roster: EQ, WoW Classic, TBC, WotLK, and future games with adapters.
- Left pane: search; **All worlds**; per-realm entries; saved collections such as Gathering crew and Evening party. These are filters/collections, not separate applications.
- Main tabs: **Roster / Map / Missions / Inventory / Activity**. Filters, selection, and selected world carry across tabs where meaningful.
- A collection may contain EQ and WoW bots. An actual in-game party can contain only bots valid for the same server/world and its party rules. A combined mission portfolio can coordinate several world-local missions; it does not create a cross-game party.
- Every row includes game/edition and realm. Same character names on different worlds remain distinct stable IDs.
- No bots are offered for an unsupported game. A client-only adapter cannot acquire bot abilities through a UI toggle.

### Roster page

Header: **Bots**, summary such as **6 active · 2 offline · 2 worlds**, Add characters, and a compact world filter. A connectivity indicator reports stale/disconnected servers separately from bot state.

Table columns: selection, Character, World, Group, Current activity, Progress, State. Character includes a small class/role icon, level, and name. World includes game and realm. Activity states what the server reports: “Mining Copper Vein,” “Travelling to Thelsamar,” “Quest: collecting objectives,” or “Awaiting assignment.” A bot with no detailed telemetry shows “Activity unavailable,” not a guessed action.

Selecting a row reveals a compact detail area below the roster: health/resource if supported, zone, current task and subtask, last update, bags used, and latest event. Multiple selection replaces this with a bulk action bar: **Activate**, **Group**, **Split group**, **Assign mission**, **Pause**, and a secondary **Stop bots** menu. Show the eligible target count; explain mixed-world restrictions before applying group operations.

**Add characters** opens an owned-character picker grouped by game/realm. It shows current human/bot/offline state, bot support, server limits, and required authorization. The final action is **Activate N bots**. Offline characters can be added to a saved collection without activating them.

Bot lifecycle: Offline → Activation pending → Active → Pausing / Deactivating → Offline, with explicit Failed and Unknown states. Active task status is a separate field: Idle, Travelling, Working, Combat, Resting, Blocked, Returning, Completed. “Unknown” after losing telemetry is not “Offline.” Stop defaults to safe deactivation/camp, with the adapter's checkpoint semantics visible.

### Map page

Use a functional zone map, not landscape artwork. Header selectors: Game / Realm / Zone / Instance / Floor. One map depicts one coordinate space. The all-world view offers zone counts or small map previews; it never plots EQ and WoW coordinates on the same invented map.

Controls: zoom, pan, center selection, follow selected bot, group coloring, trails, and objective overlays. Pins show short names and activity on selection. Distinguish current positions from stale last-known positions with a timestamp and different pin treatment. Trails expire and have a bounded sample history.

Below the map, an activity strip reports concrete events and objective changes. Example: “Brannoc mined Copper Ore +2”; “Thistle following Brannoc”; “Copper route 24/40.” Health, combat, and target details appear only when the server supplies them.

Map assets and coordinate transforms are versioned by game build, zone, instance, and floor. The adapter supplies coordinate conventions, orientation, valid bounds, and zone transitions. The app does not infer navigation paths from a bitmap. Clicking a destination opens a Move plan only if navigation commands are supported; otherwise the map is observational. Mockups use a clearly labelled schematic zone preview, not a claim to exact game-map geometry.

### Missions and groups page

A **mission** is a persisted bounded objective with assigned bots or a world-local group, preconditions, stop rules, and observable progress. It is distinct from a raw command and from a promise that an AI can solve any quest.

Mission types:

| Type | Parameters | Required server support |
| --- | --- | --- |
| Questing | Quest IDs or policy, allowed zone, reward/turn-in policy, stop condition | Quest state, supported quest execution, travel |
| Gathering / tradeskill farming | Resources or drop items, region, quantity, bag reserve | Relevant collection/combat actions and item accounting |
| Profession / crafting | Recipe or skill target, count, material budget, station policy | Known recipes, materials, station access, crafting execution |
| Mob grinding | Allowed creature set, region, level/risk bounds, duration/XP/item goal | Target selection, navigation, combat, loot |
| Fishing | Water region, skill/equipment requirements, time or item goal | Server fishing implementation and inventory reporting |
| Support / escort | Leader/group, role, follow/assist constraints | Supported group and role behavior |

The main Missions view has a cross-world table of active, queued, blocked, and completed missions. Select a mission to edit its world-local objective and inspect assigned bots, steps, events, progress, and stop rules inline. **New mission** opens a sheet with type, world, eligible bots/group, objective/region, resource budget, stop conditions, and a validation summary. Keep a simple default form; reveal detailed behavior under Advanced.

Supported sub-missions can be split from a group: choose bots, choose “temporary detachment” or “leave group,” assign the task, and set return-to-group behavior. Party membership and mission assignment are separate records. Splitting a group pauses or replans a mission that requires its healer/tank; the server validates role constraints. Do not leave a silently impossible objective running.

Stop conditions include completed quantity, maximum time, low funds, minimum bag reserve, durability/equipment threshold where available, excessive deaths, unreachable objective, required member lost, and human takeover. Progress counters derive from events and authoritative inventories/quest state, not time-based animation. Report “No progress for 8 min” or “Blocked: bags full” instead of claiming success.

Mission state: Draft → Validating → Queued → Running ↔ Paused / Blocked → Completed / Cancelled / Failed. Acknowledged server command and achieved objective are separate events. Navigation and game AI stay on the server. The mission supervisor may translate a high-level plan into supported server commands; unsupported autonomous behavior is shown as unavailable until implemented.

Missions persist on a server-side supervisor so they can continue while the launcher UI is closed. Configuration explicitly states whether closing/disconnecting only stops observation or also requests pause. A mission remains assigned to one controller lease; reconnect restores observation instead of starting a duplicate. If a server cannot provide durable supervision, label missions **Launcher must remain connected** and offer only the behavior it actually supports.

### Inventory and trade page

One cross-world inventory search can locate items across owned bots. Inventory actions remain scoped to the correct game, realm, and character. EQ items do not become WoW items. No cross-game item conversion or teleportation is implied.

Inside the main detail pane, use two inventory panels for a selected source and optional recipient. These are internal task panels inside the established two-column shell. Each panel has bot selector, world/realm label, equipment/bags tabs, item search, capacity, and snapshot time. Offer both a slot grid and an accessible list. Selecting an item exposes **Move**, **Split stack**, **Equip / Unequip**, and **Trade** only when supported. Drag-and-drop is a shortcut, never the only interaction.

**Within a bot:** move/swap/split/equip validate slot type, stack limits, unique/lore restrictions, bags/containers, combat locks, and game-specific rules. Show pending placement until the server acknowledges; re-read the final state after a conflict. A read-only adapter displays items with an explanation rather than pretending a drag changed them.

**Between bots:** the recipient selector only offers eligible same-world targets. A trade plan includes exact item instance IDs, quantity, source and destination, method, relevant restrictions, capacity, rendezvous/travel if required, and expiration. Direct trade, mail, shared storage, or an explicit server inventory service are different methods with different rules. Only display methods the server exposes. Normal trade rules still apply to bound/no-drop items, distance, faction, money, and online state.

Default UI: choose items → **Review trade** → a concise transfer plan → **Queue trade** or **Trade now**, depending on the validated method. Example: “10 Copper Ore · Brannoc → Thistle · Trade at Thelsamar.” If they must meet, show Travelling / Ready / Trading / Confirmed. Cancelling before commitment releases reservations. Do not show an Undo button for an already completed trade unless the server implements a valid reversal.

Transactions require server-side validation, item/snapshot revisions, idempotency keys, short-lived reservations, and an authoritative receipt. Never implement trades as two unrelated database updates or copy-and-delete operations in the launcher. If a request times out after committing, query the receipt before retrying. Do not promise cross-world atomic transactions or roll back a legitimate world state by overwriting an old inventory snapshot.

Loot reservations and mission materials are visible when relevant. Moving a quest item, equipped tool, or reserved crafting ingredient warns that the mission will pause/replan. Selling, destroying, mailing, and spending money are separate scoped commands; they are not hidden side effects of “organize bags.”

### Server adapter contract

A bot integration needs an authenticated server-side agent/API. A client game bridge or read-only database snapshot is insufficient to provide autonomous control. No game server administration credentials are embedded in a launcher preset or exposed to scripts/MCP.

| Capability | Example operations / data |
| --- | --- |
| bots.characters | List eligible owned player characters and companion entities with explicit identity types |
| bots.lifecycle | Activate, safely deactivate, lease control, hand off to a player |
| bots.telemetry | State, location, current activity, health/resources, freshness, event stream |
| bots.groups | Create group, assign roles, add/remove members, split/merge under world rules |
| bots.missions | Validate objective types, assign, pause, resume, cancel, report progress |
| bots.inventory.read | Versioned equipment, bags, item instances, quantities, restrictions |
| bots.inventory.write | Validated slot moves, equip/unequip, split/merge |
| bots.trade | Plan, reserve, execute, inspect receipt, cancel before commitment |
| bots.navigation | Validate destination/region and issue supported movement objectives |

Every response includes realm/build/adapter identity. Telemetry uses stable bot IDs, server time, sequence/revision, source, current objective/subtask, zone/instance/floor, and data age. Events can be coalesced for map rendering; reconnect uses a cursor or fresh snapshot and detects gaps. Snapshot persistence is bounded; the UI does not need every AI tick.

Bot commands carry explicit target IDs, expected lease/revision, schema-checked parameters, request identity, and current grants. The server checks ownership and eligibility again at execution. Expose operations through an adapter-specific agent; never hand arbitrary SQL or unrestricted remote console commands to the normal GUI, Lua runtime, or MCP clients.

### Existing systems versus required new work

CMaNGOS Playerbots explicitly supports using alt characters as bots and configurable behaviors. Its configuration includes automatic quest behavior and bot-related operational controls. This is a useful foundation, not a ready-made cross-game mission and inventory API.

EQEmu documents bot creation/spawn, group targeting, reporting, and inventory commands. Its documentation also warns that some features may be incomplete or disabled. This supports investigating a server adapter; it does not establish parity with CMaNGOS or autonomous existing-character questing/crafting/fishing.

Required new work therefore includes capability discovery, authenticated agents, durable mission orchestration where absent, character-control handoff, sufficiently rich telemetry, and transaction-safe inventory/trade endpoints. An adapter can initially expose roster observation and activation, then add map, missions, and inventory capabilities independently.

The mission planner need not be an LLM. Use deterministic validated tasks and the server's existing AI first. An optional AI assistant can propose plans through the same typed operations; it gains no extra authority by being an AI.

### Bots in scripts and MCP

Extend the common model with **Bots, BotGroups, MissionPlans, MissionRuns, Inventories, and TransferPlans/Receipts**. Proposed operations include bots.list, bots.activate, bots.pause, bot_groups.update, missions.plan, missions.start, missions.inspect, inventory.inspect, inventory.plan_move, inventory.commit_move, trades.plan, trades.commit, and trades.get_receipt.

Separate grants for **Observe bots**, **Control bots & missions**, and **Move items & trade**. A general “Manage launches” grant does not authorize inventory mutation or starting bot missions. The GUI shows the same plans, jobs, progress, and receipts regardless of whether the initiating actor was a person, script, or MCP client.

## 12. Data model and service architecture

### Core records

| Record | Responsibility |
| --- | --- |
| Game | Identity, title, artwork, metadata, adapter family |
| Release | Edition, exact client build/hash, required packages, compatible realms |
| Install | Managed path, installed release, health, package receipts, storage use |
| Realm | Endpoint, release requirements, connection method, optional character source |
| HouseholdProfile | Local preferences, enabled features, account visibility, grants |
| AccountRef | Game/realm identity and credential-store reference; no password in presets |
| CharacterSnapshot | Source identity, timestamp, field freshness, game-specific data |
| LaunchProfile | Display/config options, optional tool set, instance isolation defaults |
| SessionPreset | Slot roster, layout, input groups, launch sequencing, script bindings |
| SessionRun / Instance | Resolved immutable plan, runtime state, process generation, window IDs |
| BotCharacter / BotRuntime | Original character or distinct companion identity, realm, lease, bot lifecycle, telemetry freshness |
| BotCollection / BotGroup | Cross-world saved selection versus actual world-local party and roles |
| MissionPlan / MissionRun | Validated world-local objective, assignment, budget, stop rules, progress, durable supervisor |
| InventorySnapshot / TransferPlan / Receipt | Item identities and revisions, validated movements, reservations, committed server results |
| Package / Tool | Versioned manifest, compatibility, dependencies, conflicts, configuration |
| ScriptRevision / ScriptRun | Code, declared capabilities, revision hash, targets, output |
| ConnectionGrant | Authenticated client, operation/target scopes, expiry, review policy |
| Job / ActivityEvent | Durable operation identity, state, progress, causality, result |

### Components

1. **Desktop UI:** renders records and jobs, manages selections and forms, never patches files directly.
2. **Local broker:** owns catalog state, validation, permissions, jobs, launch orchestration, and activity. Runs within the household user's session; no always-elevated service by default.
3. **Package worker:** downloads, verifies, stages, installs, journals, repairs, and restores releases and tools.
4. **Process/window worker:** launches slots, observes lifecycle, discovers windows, applies validated layouts, and manages focus/input routing.
5. **Game adapters:** declare build matching, login/launch behavior, supported capabilities, configuration isolation, and optional bridge integration.
6. **Script worker:** scoped Lua runtime and event subscriptions; uses broker operations.
7. **MCP bridge:** exposes allowed broker operations and read models to authenticated external clients.
8. **Server bot agents and supervisor:** authenticated per-world services for character leases, bot commands, telemetry, durable missions, and inventory transactions. These operate near the game server and do not depend on local game processes.

Separate workers can fail and restart without taking down the UI or all game clients. Operations are idempotent where practical. Job requests use idempotency keys; restarting the UI does not launch a second fourteen-client session.

### Storage and isolation

Keep immutable client content separate from writable per-person, per-profile, and per-slot configuration. Use build-compatible overlays or managed copies according to what each legacy game can tolerate. Do not assume every game supports redirected configuration paths or shared executable directories.

Store secrets in the operating system credential facility. Presets export references or placeholders, never passwords. Exported diagnostics redact credentials and private endpoints by default. Shared PCs still need appropriate OS account controls; a cosmetic launcher profile is not a security boundary against another person using the same OS account.

Catalog metadata, presets, snapshots, and jobs can use a local transactional database. Packages and game files live on configured volumes. The launcher does not require cloud synchronization.

### Error vocabulary

Use specific recoverable states: Build mismatch, Required component missing, Tool conflict, Login needed, Window not found, Adapter unavailable, Capability unsupported, Permission needed, Disk full, Server unreachable. Include affected game/slot and the next action. Avoid a generic “Something went wrong” modal for an entire session.

## 13. Supporting interfaces

**Accounts:** per-game/realm identities, credential status, Test where supported, remove reference, and edit display label. Never show a password as a normal table cell.

**Media:** screenshots, captured videos, and soundtrack files belonging to the selected release. Media is optional preservation material and does not occupy the Play page.

**About:** original game metadata, preserved edition/build, local notes, source references, and adapter/manifest versions. Keep operational setup elsewhere.

**Settings:** household profile, storage, features, defaults, accessibility, credentials, package sources, and advanced diagnostics. Curator settings are visually separate from everyday launch preferences.

**Activity:** persistent jobs with progress and cancel/retry at safe boundaries; filters by game/session; readable logs with redaction. Downloads can continue while browsing another game.

## 14. Visual system

### Shared geometry and components

- 8 px spacing grid; content gutters 28–32 px; section spacing 24 px.
- Modern sans-serif UI text, approximately 14–16 px body and 12–13 px secondary text at the reference size. Serif typography is reserved for brand and selected large headings in MMOnomicon.
- 36–40 px inputs/buttons, 44 px primary launch action, 44–52 px table rows. Compact library rows are 64 px because they contain edition labels.
- One quiet filled primary action per local task. Secondary buttons have a one-pixel border. Text links serve tertiary actions.
- No duplicate ellipsis and dropdown for the same game menu. No decorative separators that resemble scrollbars or controls.
- Consistent focus outlines, clear selected state, disabled reasons, named form labels, keyboard access, and status icons with text. Meet WCAG AA contrast targets; verify actual colors in implementation.
- Main detail areas are nearly flat under text. Texture is subtle at 100% scale. No muddy gradients, luminous edges, thick fantasy frames, or distressed input controls.
- Native-like window chrome and complete application boundaries appear in every image. No device mockup or presentation caption around the app.

### Skin tokens

| Token | Ankh | MMOnomicon |
| --- | --- | --- |
| Brand | ANKH with small monoline ankh | MMOnomicon with tiny closed-book resurrection-loop mark |
| Base | Cool charcoal #181B1C | Warm charcoal #211F1B |
| Raised surface | #202526 | Dark taupe #2A2822 |
| Text | Warm ivory #E9E5DA | Bone ivory #E7E0CF |
| Secondary | #A4AAA7 | Ash #ADA79B |
| Accent | Muted antique gold #C4A565 | Muted verdigris #91AAA0 |
| Border | #373D3C | #454239 |
| Texture | Extremely fine stone/papyrus at outer edges | Fine cured-hide/vellum pores, most visible on shell edges |
| Decoration | Tiny incised line detail and ankh mark | Fine binding seam and dry matte grain |

These are starting design tokens. Text contrast and interactive state colors require measurement in a real UI. Status colors remain semantically consistent across skins.

**Ankh:** ancient Egyptian influence is limited to the mark, warm gold, proportions, and a faint edge texture. No pharaohs, pyramids, wall paintings, or ornamental hieroglyph paragraphs.

**MMOnomicon:** a lightly skin-bound grimoire of resurrection. Understated, dry, tactile, and well appointed. No gore, faces in leather, eyeballs, stitched flesh, skull decorations, claws, or exaggerated book hardware. The left border is a clean matte one-pixel edge with a faint inward shadow. Spell the wordmark exactly **MMOnomicon**.

## 15. Image set: twenty complete application screens

Each of ten screens is rendered once in Ankh and once in MMOnomicon. Match layout and sample data between skins so the viewer can compare the actual product. These are visual concepts; the Markdown remains authoritative where generated text or tiny details differ.

| Screen | Selected state | What must be visible |
| --- | --- | --- |
| 01 · Download and play | Library → WoW Classic → Play | Seven compact searchable rows; realm/account; readiness; required fixes quietly included; Install/Play hierarchy; no hero art |
| 02 · Character viewer | Library → WoW Classic → Characters | Account/realm filters; timestamped roster; Merrin, level 60 human priest, selected; compact equipment details; source/freshness; honest launch action |
| 03 · Required and optional tools | Library → EverQuest → Client tools | Required checked rows without toggles; optional Borderless, Stability bundle, Background frame limit, Game bridge; per-profile scope; one pending Apply action |
| 04 · Mixed-game multiboxing | Sessions → Mixed worlds → Layout | 14 clients / 3 games; monitor layout; 12 EQ + 1 WoW + 1 MCO; EQ-only input group off; preflight; Review & launch |
| 05 · Scripting and abstraction | Automation → Arrange on launch → Editor | Script list; typed API context; short Lua script; session target; Run; output; capability distinction between EQ bridge and MCO window-only |
| 06 · MCP integration | Connections → Local MCP server | Local server state; copy configuration; client grants; bot permissions distinct from launch permissions; recent requests; revoke control; no chatbot |
| 07 · Cross-world bot roster | Bots → All worlds → Roster | Eight EQ/WoW characters; six active, two offline; current task and progress; two compatible bots selected; Group / Split group / Assign mission; Activate action |
| 08 · Zone tracking | Bots → Arcanite → Map | Schematic Loch Modan map; Brannoc and Thistle pins; objective/trail controls; freshness; actual task/activity strip; world/zone selectors |
| 09 · Missions and groups | Bots → All worlds → Missions | EQ and WoW mission table; selected Copper route; gathering/grinding/crafting/fishing/questing choices; assigned group and split sub-mission; bounded goal and stop rules |
| 10 · Inventory and trading | Bots → All worlds → Inventory | Cross-world search/filters; source Brannoc and recipient Thistle on Arcanite; bag move/split/equip controls; 10 Copper Ore trade plan; rendezvous; Queue trade action |

### Exact mockup data

- Profile: **Household**. WoW Classic realm: **Arcanite**. Account: **Main account**. Build: **1.12.1**.
- Character sample: Merrin — 60 — Human Priest — Ironforge; Brannoc — 42 — Dwarf Hunter — Loch Modan; Thistle — 28 — Night Elf Druid — Ashenvale. Source line: **Realm snapshot · Updated 2 min ago**.
- Character equipment example: Benediction, Robes of the Exalted, Devout Sandals. These are illustrative item names, not a verified character inventory.
- EQ tools sample: **Required for this client · 3 ready**. Connection setup, Legacy runtime, Client compatibility. Optional tool labels are descriptive placeholders, not claims about specific released packages.
- Session: **Mixed worlds**. Summary: **12 EverQuest · 1 WoW · 1 Motor City Online**. Layout: **Main + auxiliaries**. EQ group: **EQ party · 12 targets · Input off**.
- Automation script: **Arrange on launch**. Session target: **Mixed worlds**. Output example: **Validated 14 targets**; **Layout applied to 14 windows**. Capability badges: **EQ · live bridge** and **MCO · windows only**.
- MCP client sample: **Household assistant**, connected, grant **Manage launches**; **Local inspector**, disconnected, **Inspect only**. Recent calls: library.list, session.preflight, windows.apply_layout. States and activity are mock data.
- Bot sample: EQ realm **Norrath**, WoW Classic realm **Arcanite**. EQ: Aelric, Seren, Tovin active and Dorrin offline. WoW: Brannoc, Thistle, Merrin active and Rook offline. Total **6 active · 2 offline · 2 worlds**. These examples assume a future capable EQ server agent, not stock feature parity.
- Missions: **Silk supplies** — EQ/Norrath — Aelric + Seren — 18/40; **Copper route** — WoW/Arcanite — Brannoc + Thistle — 24/40; **Profession work** — WoW/Arcanite — Merrin — 3/10. Tovin is awaiting assignment.
- Bot map: **Loch Modan · Schematic preview**. Brannoc: Mining Copper Vein. Thistle: Following Brannoc. Last telemetry: **Updated 1 sec ago**. This is concept data.
- Inventory trade: **10 Copper Ore · Brannoc → Thistle · Arcanite**. Method: **Trade · Meet at Thelsamar**. Plan state: **Ready to queue**. No cross-world transfer appears.

### Rendering brief shared by all twenty images

Create a polished, believable desktop application screenshot, straight-on and uncropped, wide landscape, with crisp typography, aligned controls, restrained density, and two primary columns. Use the geometry, screen contents, and skin tokens above. One full screen per image. No hero scene, promotional cards, giant character illustration, decorative fantasy frame, or floating UI fragments. Use small game-era-appropriate library icons only. Keep interface labels conventional. Every table has proper headers, every dropdown a clear label, and every action a plausible location. Avoid fake graph filler and decorative code walls. Image generation is the rendering method; this specification supplies the behavior.

## 16. End-to-end behavior and acceptance criteria

### Basic play

- A fresh profile can install and launch a game without visiting Client tools, Sessions, Automation, or Connections.
- Required dependencies are included in the install plan and readiness checks. Broken required setup blocks launch with a repair action; optional tooling does not.
- Downloads resume after restart. Disk-full and verification failures leave the prior usable release intact.
- Import never overwrites an unmanaged client without a reviewed plan. Unknown builds are identified as unknown.

### Optional tools

- Different household profiles can use different optional tools where the adapter supports isolation; shared-file limitations are stated.
- Dependency/conflict resolution is previewable. Disabling a tool restores its configuration or switches to an unmodified managed variant.
- Exact build and package versions are recorded. A pending tool update cannot mutate files under running clients.

### Characters

- Cached data includes source and time. A stale snapshot remains readable and clearly stale.
- Unsupported or failed data sources do not produce invented inventory, levels, or live statuses.
- Launch action wording reflects whether the adapter selects a character or merely the account.

### Mixed-game session

- The specified 12 EQ + 1 WoW + 1 MCO roster remains fourteen independent slots throughout save, launch, crash, and retry.
- Layout saves, previews, and restores all fourteen assignments. Removing a monitor produces a recoverable mapping problem.
- One failed slot can retry without restarting other clients. Closing the launcher UI does not silently close games.
- EQ input targets never include WoW/MCO unless explicitly configured. Master pause works without the main window focused.
- Performance checks include a representative long-running fourteen-client session on target household hardware, not just an empty-window demo. Resource estimates are calibrated from observed builds.

### Automation and MCP

- The same operation produces the same validation/result through GUI, script, and MCP.
- Missing capabilities are inspectable and fail explicitly. Runtime cancellation stops script work and pending input, not unrelated game processes.
- Revoked grants cannot make new calls. Secrets are absent from exports and logs.
- A reconnect can inspect a durable job/run rather than repeating the launch.

### Server bots

- EQ and WoW bot states appear in one roster, while groups, maps, and trade targets preserve world identity.
- Activating an actual character acquires a server lease; handoff to a human releases bot control before client login. Companion bots remain explicitly distinct identities.
- A bot can continue on its configured mission without a local game window. Closing the launcher follows the configured persistence policy, visibly and predictably.
- Map disconnects show stale positions. Reconnect detects missing events and obtains a fresh snapshot.
- Group splitting validates remaining mission roles. Unsupported questing, farming, crafting, grinding, or fishing appears unavailable with a reason.
- Quantities and completion reflect authoritative world events. Acknowledging a command is not completing a mission.
- Inventory conflicts refresh/revalidate; timed-out trades query receipts before retrying. Tests cover partial failures, full bags, bound/no-drop items, duplicate requests, and loss of connection after commitment.
- The bot feature is independently optional. Library-only users never need to configure a bot server or mission.

### GUI quality

- Test with seven games, hundreds of releases, long edition names, no installed games, and no search results.
- Keyboard-only navigation, high scaling, reduced motion, and screen-reader labels are implemented.
- Main actions stay visible at the minimum size; tables scroll instead of shrinking into illegibility.
- Both skins use the same components and behaviors. Texture never reduces readability.

## 17. Delivery sequence

1. **Foundation:** catalog/manifests, managed installation, required setup, accounts, launch, Activity, and adapter capability contract.
2. **Maintenance and viewing:** optional tools with transactions/rollback; character snapshots where data sources exist.
3. **Mixed-game sessions:** stable slots, config isolation, staggered launch, monitor layouts, runtime controls, then scoped input management.
4. **Automation:** Lua runtime, shared object model, inspector, GUI control declarations, and incremental per-game live adapters.
5. **Bots:** per-world server agents and ownership/lease model; roster/activation; telemetry/maps; bounded missions/groups; validated inventory/trade. Add one capability at a time and expose honest adapter limits.
6. **MCP:** read models and preflight first, then scoped execution through the same broker, including separate bot and inventory permissions. Preserve the simple launch workflow throughout.

The first implementation decision to validate is each game's launch and isolation adapter. Window management can deliver value before deep game-state integrations exist. Treat deep EQ/WoW/MCO bridges as separate scoped projects with build-specific compatibility work.

## 18. Research and boundaries

The following primary sources ground the reference models. All detailed application behavior above is proposed for this launcher; it is not attributed to these projects.

- [ThwargLauncher official site](https://www.thwargle.com/) describes simple launching and profile-based multiple-account launching. This supports the familiar simple/advanced starting point.
- [ISBoxer: Window Layout](https://wiki.isboxer.com/Window_Layout) documents regions, slots, and active-region swapping. This informs the proposed separation of session slots from window regions.
- [ISBoxer: Repeater](https://wiki.isboxer.com/Repeater) distinguishes keyboard/mouse repetition and notes alignment issues between differing client interfaces. The proposed product therefore separates layout, input targets, and semantic mappings.
- [MacroQuest: Lua](https://docs.macroquest.org/lua/) and [Top-Level Objects](https://docs.macroquest.org/reference/top-level-objects/) show the scripting and inspectable-data reference model. A cross-game equivalent still needs independent adapters and schemas.
- [MCP: Tools](https://modelcontextprotocol.io/specification/2026-07-28/server/tools) and [Transports](https://modelcontextprotocol.io/specification/2026-07-28/basic/transports) ground the external tool interface and transport choices. Implementation must target the chosen SDK/protocol revision and its compatibility rules.
- [CMaNGOS Playerbots README](https://github.com/cmangos/playerbots/blob/master/README.md) describes alt-character bots and configurable behaviors. [Playerbots configuration](https://github.com/cmangos/playerbots/blob/master/playerbot/aiplayerbot.conf.dist.in) includes autonomous questing and operational options. These establish useful primitives, not a complete cross-game management API.
- [EQEmu bot command documentation](https://github.com/EQEmu/eqemu-docs-v2/blob/main/docs/server/bots/bot-commands.md) lists creation/spawn, actionable group selection, and inventory operations, with explicit notes about incomplete/disabled features. Actual-character conversion and autonomous mission parity require separate validation and server development.

Scarab's private source was not available during this pass. Its role here is the user's description: a ThwargLauncher-like foundation with additional layers. No Scarab internals, existing features, or migration compatibility are assumed.
