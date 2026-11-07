# ankh

A minimalist, robust game management application for a private collection of preserved games. It starts as a download-and-play launcher and grows, only for those who want it, into client tools, mixed-game sessions, automation, server-side bots, and MCP access.

Ankh and MMOnomicon are two skins of the same application.

- Product and interface specification: [docs/launcherSpec.md](docs/launcherSpec.md)
- Wordmark: [assets/ankhWordmark.svg](assets/ankhWordmark.svg)

## Stack

C++ with Qt, Windows first. Qt is linked dynamically and its DLLs ship beside `ankh.exe` in the package; nothing is installed separately. Qt is used under the LGPL, so the package carries Qt's license text.

## Layers

1. Play: catalog, download, import, required setup, update, launch, accounts.
2. Characters and required compatibility.
3. Optional client tools.
4. Sessions.
5. Automation.
6. Server bots.
7. MCP.

Layers 1 to 3 are the product; 4 to 7 are independently enabled features. See the specification, section 17, for the delivery sequence.
