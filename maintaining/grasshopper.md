## Grasshopper plugin

- C# / .NET project ([grasshopper-plugin/TapirGrasshopperPlugin/](../grasshopper-plugin/TapirGrasshopperPlugin/)),
  multi-targeting `net7.0-windows;net7.0;net48` (Rhino 6/7/8), output extension `.gha`.
- Components are organized by domain under `Components/` (Elements, Attributes,
  Properties, Classifications, Navigator, Issues, Teamwork, Favorites, Project, etc.),
  mirroring the Add-On command groups. Uses `Newtonsoft.Json` for the JSON interface.
- Build with `dotnet build` / Visual Studio using
  [TapirGrasshopperPlugin.sln](../grasshopper-plugin/TapirGrasshopperPlugin.sln).
- Packaged for the Rhino Package Manager (Yak) as **tapir**; see
  [grasshopper-plugin/YakPackage/](../grasshopper-plugin/YakPackage/) and
  `create_package_win.bat`.

### Naming from the Archicad GUI

Every Grasshopper component uses the names a user sees in Archicad, not the API names.
- An input, output or JSON key that stands for a setting of an Archicad tool is named after
  the English label of that setting's control in the tool's settings dialog, e.g. the wall's
  `begThickness` gets the label of its thickness control. This applies to new and changed inputs
  and outputs; existing ones are renamed when their tool component (below) replaces them.
- The labels, with the dialog panel (Group) and the panel's subheading (Subgroup), are kept per
  tool in [grasshopper-plugin/TapirGrasshopperPlugin/ToolSettings/](../grasshopper-plugin/TapirGrasshopperPlugin/ToolSettings/)
  (`<Tool>.json`, e.g. `Wall.json`), checked by `python tools/check_tool_settings.py`. Take the
  labels from there; code only uses that copy.
- Never invent a label. Add a label to the table only when it is found verbatim on Graphisoft's help
  page for that settings dialog (help.graphisoft.com, the Archicad version the table names: its text
  or its dialog screenshots) or in a DevKit comment that names the control, and name the page and
  screenshot in `labelSource`; otherwise leave it `null`, and the API name is used. Capitalization
  follows the dialog (Title Case).
- Labels are unique per tool, case-insensitively, because the JSON is translated back to API
  names by label. A field belongs to exactly one Group.

### Archicad tool components (target architecture)

Elements are handled by **one component per Archicad tool** (Walls, Slabs, Columns, …) that
mirrors the tool's settings dialog. It is introduced tool by tool, starting with Walls (prototype:
`Components/DesignToolsComponents/`, `ElementToolComponent` with `WallsComponent` and its `Walls_API`
twin); new work on an element tool follows these rules:
- **Name and place:** the plural tool name ("Walls"), in the group Graphisoft puts the tool in:
  `GroupNames.DesignTools` ("Design Tools"); `DocumentTools` and `ViewpointTools` are added with
  their first component.
- **Built from the table:** a tool component derives from `ElementToolComponent`, which builds its
  JSON inputs and outputs from `ToolSettings/<Tool>.json` (embedded in the plugin); the subclass
  only names the tool, its Create/Modify commands with the keys each accepts, its geometry
  inputs and its dialog panels (`JsonGroups`). Saved definitions find parameters by position, so
  the JSON parameters come last and `JsonGroups` is append-only: never reorder or remove a panel.
  Execute checks the element type first (the Modify commands do not), and `PrepareSettings` drops
  values the commands cannot write, so a JSON read by Refresh can always be fed back.
- **Three buttons, one set of JSON objects:** *Refresh* reads (`GetDetailsOfElements`), *Execute*
  writes, *Create* makes new elements (the tool's `Create<Type>` command, starting from the tool
  defaults or the `FavoriteName` input). A tool component may remember the elements its *Create*
  made (saved with the .gh file; Walls does, `CanUpdateCreatedElements`): the next *Create* then
  modifies them instead of making new ones, and reports one that was deleted as an error rather
  than recreating it (#521). Writing and creating only happen on a button press; the
  component ignores `AutoExecute`, only *Refresh* may run automatically. Clear the request flags
  inside `SolveInstance`, where they are used, not after `ExpireSolution (true)`.
- **One JSON input and output per dialog panel:** the Group name without spaces + `_JSON`
  (`GeometryAndPositioning_JSON`, `FloorPlanAndSection_JSON`, `Model_JSON`,
  `ClassificationAndProperties_JSON`), keys are the GUI labels from the table. Only the keys given are
  written (partial updates), arrays are replaced, not merged. With N elements each JSON input holds
  0 (unchanged), 1 (all) or N items; anything else is an error.
- **Plain inputs/outputs** for what is not on a dialog panel: `ElementGuid`, `ElementID`,
  `LayerIndex`, `DrawOrder`; and **every input that defines the geometry** (drawn in Archicad), e.g.
  `StartPoint`/`EndPoint` for walls, which override the JSON on *Create*.
- **Execute and Create accept the same keys:** on the add-on side they share one apply-function
  (walls: `ApplyWallDetails`, used by `CreateWalls` and `ModifyWalls`; `SetDetailsOfElements`'s
  wall case only takes coordinates, height, offset and the thicknesses of trapezoid walls).
- **Delete is never part of a tool component**; it stays in the separate `DeleteElements`
  component.
- A `<Tool>_API` twin with the raw API names may exist for testing.
- The old per-command components stay until the tool component reaches parity, then get
  `Exposure` `GH_Exposure.hidden` for one release and are removed in the next. Keep a component's
  GUID when moving it. Archicad attributes (building materials, surfaces, composites, …) will follow
  the same pattern later.
