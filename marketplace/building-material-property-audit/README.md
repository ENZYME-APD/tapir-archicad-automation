# Building Material Property Audit

Lists the value of one custom property for every Building Material of the project, so the
materials with an empty value stand out. You can export the list, or give the materials you
choose a value.

## How to use it

1. Open the project and run the script. It reads the Building Materials and the custom
   properties that are available for them.
2. Pick the property in the *Property* list. The window lists every Building Material with its
   folder and its value: an empty value in red, `(not available)` in grey when the property is
   not available for that material. *Only materials with an empty value* hides the others.
3. To fill in a value, type it in *New value* and press *Set value...*. It goes to the selected
   rows, or, with no row selected, to every listed material with an empty value. The script
   shows which materials it will change and asks first; materials whose value cannot be edited
   are skipped and counted.
4. *Export...* saves the listed rows as an Excel workbook (`.xlsx`) or a CSV file.
5. *Refresh* reads everything again, for example after you changed the properties in the
   Property Manager.

## Settings

- *Property*: the custom property to check; the first one in the list at the start.
- *Only materials with an empty value*: off.
- *New value*: empty.

## What it changes

*Set value...* writes the value of the chosen property to the chosen Building Materials, after
you confirm. Materials it could not change are named in the palette's output, and the first
one in the window's status line. Reading and exporting change nothing in the project.

*Export...* writes one file, where you choose in the save dialog; its default name is
`Building Materials <date> <time>.xlsx`.

## Limits

- Only custom (user-defined) properties are listed, and only those available for at least one
  Building Material.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- In a Teamwork project, a Building Material you have not reserved may not be changed; the
  ones that could not be changed are reported.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #1992, written with the Tapir issue bot, based on the
`audit_building_material_properties.py` workflow example.
