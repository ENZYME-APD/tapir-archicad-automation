# Custom Layers for Views

**macOS only, and only with Archicad's interface in English.** Tested on Archicad 29
alone. It is a stand-in for something the Archicad API cannot do, not a Tapir
command: it presses Archicad's own buttons for you.

Stores custom layers in views of the View Map: each view keeps the layers it shows
now, with the layers you choose shown, hidden, locked or unlocked. A view that uses
a layer combination gets the combination's layer states as its own custom layers.

The Archicad API cannot write a view's custom layers (see
[What the Archicad API can't do](https://archicad-api.com/limits/)), so the script
does what you would do by hand, view by view: it opens the view, sets the layers in
the window and presses **View Settings > Get Current Window's Settings** in the
Navigator. It does this through macOS Accessibility, without bringing Archicad to
the front and without using your pointer or keyboard.

## How to use it

1. In Archicad, show the Navigator docked in the project window, with its
   Properties panel open (the panel with the view's ID, name and **Settings…**).
2. Run the script from the Tapir palette. The window lists the View Map's views in
   their folders, with how each holds its layers: **Custom**, the name of a layer
   combination, or **not stored**.
3. Select the views to change. Selecting a folder selects its views.
4. Select layers in the lower table and press **Show**, **Hide**, **Lock** or
   **Unlock**; **No change** clears it. Layers you leave alone keep the state each
   view has for them. With no layer changed, a view only goes from its layer
   combination to custom layers with the same states.
5. Press **Store custom layers...** and confirm. The Result column says
   **Stored** and what changed in each view, or why a view failed.

Typical use: after making a new layer, select the views that must show it, set the
layer to **Show** and store. (A new layer is hidden in every view that was saved
with custom layers before it existed.)

Leave Archicad alone while the script runs.

## Settings

None.

## What it changes

- It changes the model: the layer settings stored in the views you select. The view
  stays the same Navigator item, so Drawings placed from it keep their link.
- **Get Current Window's Settings** stores every setting of the window, not only
  its layers. The script therefore reads the view's settings first, writes them back
  afterwards (zoom, scale, pen set, model view options, graphic overrides,
  renovation filter, ...), and reads the view again. A view passes only when it
  stores custom layers equal to the window's, layer by layer, and every other
  setting and its design option combination are as before.
- A view that fails after it was overwritten is put back as it was: a layer
  combination is written back, custom layers are stored again from the layers the
  view opened with. The Result says whether the view is unchanged, was put back, or
  is still changed and how. The script stops at the first view that fails.
- It opens each view in Archicad, which replaces the layers and zoom of the window
  you have open there, and leaves Archicad on the last view.
- Archicad's Undo does not take it back.
- It writes no files.

## Limits

- macOS only. On Windows it says so and stops.
- Archicad's interface in English only: the script finds four controls by their
  English names, and stops with a message when it does not find one. Their names
  are constants at the top of the script; another language's names there are
  untried.
- The app the script runs in needs Accessibility (System Settings > Privacy &
  Security > Accessibility).
- The project must be saved (the script finds its Archicad by the project's file),
  and the Navigator docked with its Properties panel open.
- Tested only on Archicad 29 (build 5101) on macOS 26, with floor plan, section
  and 3D views, in a solo project. Other Archicad versions, Teamwork and other
  kinds of view are untried. It needs a Tapir Add-On that returns `customLayers`
  in GetViewSettings (2.0.2 or later).
- It depends on how Archicad 29 lays out the Navigator and the View Settings
  dialog, so an Archicad update can break it; it then stops with a message.
- Of a view's design option combination it compares the status and the name only.
- After a view with custom layers was put back, it also stores the layers made
  since it was saved, hidden, where before they were absent from it; it shows the
  same.

Original by Ben Tristem.
