# HUD overlays

## Monster information (F8)

`CNewUINameWindow` owns the optional monster overlay. Pressing F8 toggles it for all visible, living monsters. The client requests an authoritative snapshot from the server every 750 ms so the label can show:

- monster name;
- monster level;
- current and maximum health; and
- a compact health bar.

An unknown health value is rendered as `HP --` with a full placeholder bar until the server supplies the first snapshot.

## Quest tracker (F7)

The quest tracker starts on the right side of the gameplay HUD. Drag its header to move it. Pressing F7 hides or shows the complete tracker, and the X button also closes it. Its arrow collapses or expands the list, and the whole panel consumes mouse input so clicking it cannot move the character.

The `All` tab shows every active quest, including quests for other maps. `This Map` filters the list to quests relevant to the current map. When more than five quests match, the mouse wheel scrolls the list.

The tracker reuses the Season 6 extended quest system (`CQuestMng`) and therefore includes every active quest delivered by the normal active-quest list, including quests accepted through NPC dialogue. It requests live quest state every 2.5 seconds so kill and item counts stay current.

Map filtering uses the unencrypted `0xF5` custom group:

- client request: subcode `0x09`;
- server response: subcode `0x0A`, protocol version `1`;
- response header: map number and quest count;
- each record: quest group and quest number.

The server considers a kill quest relevant when at least one required monster is configured to spawn on the player's current map. Active quests without monster-kill requirements stay visible, which prevents item, level, tutorial, event, and other custom objectives from disappearing just because they have no direct map metadata.

The client discards a response whose map number no longer matches the active map, preventing a delayed packet from applying after a map change.
