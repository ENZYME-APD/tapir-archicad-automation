# Archicad tool settings tables

One JSON file per Archicad tool (`Wall.json`, …). Each file lists every setting of
that tool which the Tapir Add-On reads or writes, and ties it to the setting's
place in the tool's settings dialog in Archicad. The Grasshopper tool components
use these tables to show the names users know from Archicad instead of the API
names (see "Naming from the Archicad GUI" in the repository's `CLAUDE.md`).

## Fields

| Property | Meaning |
| --- | --- |
| `get` | Key in the `GetDetailsOfElements` response (`details`, or the element itself when `getFrom` is `element`) |
| `set` | Key of the tool's Create/Modify commands (e.g. `CreateWalls`, `ModifyWalls`) |
| `apiMember` | The Archicad API member behind it, e.g. `API_WallType::thickness` |
| `apiDescription` | The DevKit's description of that member (`Support/Inc/APIdefs_Elements.h`) |
| `label` | The English label of the setting's control in the tool's settings dialog, or `null` when not known yet |
| `labelSource` | Where the label was found: the Graphisoft help page (URL with anchor) or `devkit:<header> <member>` |
| `group` | The dialog panel, e.g. "Floor Plan and Section" |
| `subgroup` | The subheading inside the panel |

`plainKeys` are keys of the commands that are not settings (`elementId`, `favoriteName`).

## Rules

- A label is only written when it is found verbatim on Graphisoft's help page for that
  settings dialog (help.graphisoft.com, the `archicadVersion` of the file) or in a DevKit
  comment that names the control. Never guess one; a field without a label keeps its API name.
- Labels are unique per tool, case-insensitively.
- Every key of the Add-On is in the table exactly once.

`python tools/check_tool_settings.py` checks this against the Add-On sources (no
Archicad needed); `--markdown` prints the tables for review. Run it after changing a
table or the Add-On's Get/Create/Modify code of a tool.

## Adding a tool

Copy `Wall.json`, set `tool`, `elementType`, `commands` (`getCase` is the `case API_<Type>ID:`
in `GetDetailsOfElementsCommand::Execute`, `create`/`modify` the command classes) and let the
checker list the keys it expects.
