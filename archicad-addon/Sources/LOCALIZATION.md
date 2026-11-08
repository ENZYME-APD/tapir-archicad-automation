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

# Translating the Tapir Installer

The texts of the Tapir Installer ([../Installer/tapir_installer.py](../Installer/tapir_installer.py)):
the window, its buttons and statuses, the results and error messages, the console output and the
messages of update mode, are in the `TEXTS` table at the top of that file, one table per language,
with the same language codes as the Add-On. Only English (`INT`) ships for now.

Not translated: the help of the command line options (`--help`) and the option names inside the
messages (`--versions`, `--addOnFile`, ...), the error details that come from the operating system or
from Archicad, and the lock file states `quitting`/`updating` of the update. The English texts of an
update already running (`UPDATE_RUNNING_TEXT`, `UPDATE_WAITING_FOR_QUIT_TEXT`) must stay the same as
in the legacy update script (`Tools/update_addon_and_restart_archicad.py`); `test_update_mode.py`
checks it. Their translations may say the same in the other language.

## How the language is chosen

One installer holds every language, and chooses one when it starts:

1. the `--language` option: a language code (`GER`) or a language tag (`de-DE`). Later the Add-On may
   pass its own language in update mode; installers that do not know the option ignore it;
2. otherwise the language of the operating system: `GetUserDefaultUILanguage` on Windows, the first
   of the preferred languages (`AppleLanguages`) on macOS, then the `LANGUAGE`, `LC_ALL`,
   `LC_MESSAGES` and `LANG` variables and `locale`;
3. English (`INT`) for any other language.

The language of a tag is matched by its first part (`de-AT` is `GER`), except `pt-BR` (`BRA`) and the
traditional Chinese `zh-Hant`, `zh-TW`, `zh-HK` (`TAI`); the table is `LANGUAGE_CODES_BY_ISO_LANGUAGE`.
A text that a language's table lacks is shown in English, so a table may be translated in parts.

## Adding a language

For example French, `FRA`:

1. In `TEXTS`, add an entry `'FRA' : { ... }` after `'INT'`, with the keys of the `INT` table and the
   texts translated. Keep:
   - the keys (`'buttonInstall'`, ...), only the texts are translated;
   - the placeholders `{0}`, `{1}`, ...: each one of the English text, any order (`test_update_mode.py`
     checks it);
   - `\n` (a line break) and `\'` (a quote);
   - the names Tapir, Archicad, GitHub, the option names and the web addresses.
2. Keep the statuses short (`Installed 1.2.3`, `Downloading 42%`): their badge has a fixed width.
3. Save the file as UTF-8.
4. Try it: `python tapir_installer.py --language FRA` (the window), and in console mode with
   `--console --dryRun --language FRA`. Then run `python test_update_mode.py`.

The executables need nothing else: the table is part of `tapir_installer.py`, which PyInstaller
bundles. When an English text is added, the other tables need not change: it is shown in English until
it is translated. When one is changed, change its translations too, or remove them until they are
translated again.
