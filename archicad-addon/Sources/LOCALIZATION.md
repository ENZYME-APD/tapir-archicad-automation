# Translating the Tapir Add-On

The texts of the Add-On's user interface (the Tapir menu, the Tapir palette, the About and
Script Shortcuts dialogs, the update and `uv` alerts, the messages in the Report window) are
resources in [RINT/AddOn.grc](RINT/AddOn.grc), the international English version and the
complete one. The C++ code reads the texts with `RSGetIndString` by the IDs in
[ResourceIds.hpp](ResourceIds.hpp).

Not translated: the JSON command names, their schemas and descriptions (the API documentation)
and the error messages commands return to scripts. They are not resources. The resources in
`RFIX/` (icons, the add-on identifier, the schema file) do not depend on the language either.
Two texts Archicad shows are still in the code: the names of the undo steps the commands record
(`Undo <name>` in the Edit menu, most of them the command's name) and the file type name
"Python Script" in the file dialog of *Add Script* (the file type API takes an 8-bit string).

## How the language is chosen

The Add-On follows Graphisoft's approach (the DevKit's `CMakeCommon.cmake` and
`BuildAddOn.py`, and the guide
[How to Create Multi-Language Add-On](https://archicadapi.graphisoft.com/how-to-create-multi-language-add-on)):
an add-on build has one language, chosen when it is built. The localized resources of language
`<code>` are in `Sources/R<code>/`, the CMake option `AC_ADDON_LANGUAGE` (default `INT`) picks the
folder, and `Tools/CompileResources.py` compiles that folder with `RFIX/`. The codes are the ones
of Archicad's language versions (`INT`, `GER`, `FRA`, `ITA`, `JPN`, ... as in the guide's table).

The releases are built with `INT`. How translated builds reach the users (one add-on file per
language, or the guide's Windows MUI resource files next to the `.apx`) is still open in #2119.

## Adding a language

For example French, `FRA`:

1. Copy the folder `RINT` to `RFRA` and keep the file name `AddOn.grc` (`RFIX.win/AddOnMain.rc2`
   includes it by that name).
2. Translate the quoted texts in `RFRA/AddOn.grc`, keep everything else as it is:
   - the resource types and IDs (`'STR#' ID_PALETTE_STRINGS ...`), and in every `'STR#'` list the
     number and the order of the entries: the code finds a text by its position;
   - the placeholders `%s`, `%d` and `%T`, in the same order (they cannot be reordered);
   - `\n` (a line break) and `\"` (a quote);
   - in the menu texts, the part from the first `^` on (`^E3^ES^EE^EI^ED^EL^EW^ET^EM^32503`):
     it says where the menu item goes and which icon it has;
   - in `'GDLG'` the control types, positions and sizes; in `'DLGH'` the last column (the item
     names). The middle column of `'DLGH'` is the tooltip text and is translated;
   - the names Tapir, Archicad, GitHub and `uv`, the web addresses and the file names.
3. Save the file as UTF-8.
4. Build with the language selected, e.g. for Archicad 29 on Windows:

   ```bat
   cmake -B Build/AC29_FRA -G "Visual Studio 18 2026" -A x64 -T v143 -DAC_VERSION=29 -DAC_ADDON_LANGUAGE=FRA -DAC_API_DEVKIT_DIR="Build\DevKits\AC29\Support" .
   cmake --build Build/AC29_FRA --config RelWithDebInfo
   ```

5. Load the add-on in Archicad and look at each dialog: a longer text may need a wider control
   in `'GDLG'`. Change the size in the translated file only.

When a text is added or changed in `RINT/AddOn.grc`, every translation gets the same entry at the
same position: the English text until it is translated.
