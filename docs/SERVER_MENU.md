# Server Menu

The client adds a separate **Menu** button at the upper-right of the game view.
It does not replace the classic `U` window menu.

The server menu contains these pages:

- **Events** requests the live Blood Castle, Devil Square, and Chaos Castle
  schedule from the game server. It shows a countdown, an open-entrance state,
  or an unavailable state and refreshes automatically when a countdown ends.
- **Rankings** displays a server-backed top-character table with Player, Class,
  and the selected Level, Resets, or Master Level value.
- **Reset** requests structured reset data when the page opens and displays the
  character's current and required level, Zen and reset item count, the reset
  limit, next reset number, reward points, and readiness. `/reset` and
  `/resetstats` remain second-click-confirmed actions.
- **Offline Level** opens MU Helper settings, starts MU Helper, and activates
  `/offlevel` after a second-click confirmation.
- **Commands** exposes Move, Open Warehouse, and a scrollable Available
  Commands screen. The command list is supplied by the server and includes only
  active commands authorized for the selected character's status.
  Open Warehouse uses a dedicated server request and the standard vault NPC
  action.

The window uses a code-rendered dark steel layout instead of stitched legacy
message-box textures. Its frame, buttons, tabs, and ranking rows scale from
shared layout constants, so widths and spacing can be changed without creating
new bitmap assets. A top-right **X** closes the menu directly from every page.
The ranking page uses a full-width header and ten bordered, alternating rows
for consistent column alignment.

## Offline leveling flow

1. Configure and save MU Helper.
2. Leave the safe zone and start MU Helper.
3. Click **Activate Offline Level** twice to confirm.

On success, the server disconnects the real client and continues the character
on the current map with the saved MU Helper configuration. Logging into the
account again stops the offline character automatically before character
selection.

The game server remains authoritative for command availability, permissions,
costs, and reset requirements.

## Event schedule packet

The client sends `C1 04 F5 02` when the Events page opens or is refreshed. The
game server answers with `C1 18 F5 03`, protocol version `1`, followed by the
server timestamp and the state/countdown fields for all three events. Client
and server builds containing this packet must be deployed together.

## Character ranking packet

The client sends `C1 05 F5 04 <filter>` where filter `0` is Level, `1` is
Resets, and `2` is Master Level. The server answers with `F5 05`, protocol
version `1`, and up to ten fixed-size character rows. Each row contains the
character name, class, level, resets, and master level. Sorting is performed by
the server for the selected filter. Bot/template accounts, Game Master accounts,
and Game Master characters are excluded.

Master Reset is intentionally not exposed yet because the server has no
persisted master-reset stat. The packet can be extended when that feature is
implemented instead of presenting a misleading zero-valued ranking.

## Reset requirements packet

The client sends `C1 04 F5 07` when the Reset page opens. The server answers
with `F5 08`, protocol version `1`, and the current/required reset values plus
individual readiness flags. This keeps calculation and configuration on the
server while allowing the client to render the requirements directly.

## Available commands packet

The Available Commands page sends the standard `C1 04 F5 00` command-list
request. The server returns one `F5 01` entry per active command available to
the selected character, including its localized name and description. The
client collects these entries, displays three at a time, and scrolls one entry
with the mouse wheel or Up/Down keys. The commands are alphabetized, and the slim
position bar can be dragged with the mouse to move through the complete list.

## Warehouse packet

The client sends `C1 04 F5 06`. The server opens the configured Vault Storage
window through `TalkNpcAction`, so normal NPC-state validation remains in one
place and the button does not depend on the optional `/openware` chat plug-in.
