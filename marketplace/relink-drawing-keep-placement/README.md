# Relink Drawing, Keep Placement

Relinks the Drawing selected on a layout to another view of the project, and keeps
it exactly where it was on the sheet: the same displayed scale, the same
orientation and the same frame. For example, a plan placed from a 1:500 view can
show a 1:200 view instead, without moving, rescaling or re-cropping the Drawing
afterwards.

## How to use it

1. Open a layout and select one Drawing on it.
2. Run the script from the Tapir palette. A window opens and follows the
   selection in Archicad: select another Drawing and the window shows it.
3. The window shows the layout, the Drawing's displayed scale and its source
   view. When more than one view on the layout could be the Drawing's source
   (several Drawings of the same scale with different orientations), it lists
   them and asks which one: pick it.
4. Pick the target view in the View Map tree (only views of the same family are
   offered, see Limits). The filter box searches names, folders, types and
   scales (e.g. `1:200`); folders can be collapsed.
5. Press **Relink**, or double-click the view. The window says when it is done
   and checks that the Drawing has not moved; if it finds a difference it shows
   it (in mm and inches) so you can check the Drawing.
6. **Back to "…"** relinks the Drawing to its previous view, exactly as it was
   before. It can be pressed several times in a row to go further back.

## Settings

None. The new Drawing gets:

- a magnification that keeps the displayed scale it had,
- an angle that keeps its orientation on the sheet, even when the new view is
  rotated differently,
- a position that keeps the model where it was on the sheet,
- the same frame: its clip polygon, or, when it was not cropped, its previous
  full frame as a clip polygon.

Its other settings (name, numbering, border, pen set, colours, title type,
anchor, update mode, …), the parameters of its title and the title's position
are copied back from the old Drawing.

## What it changes

- It changes the model: the selected Drawing is relinked to the view you pick
  (Archicad replaces it with a new Drawing element), and its settings and title
  are set as above. Nothing else is changed.
- A relink takes several Tapir commands, so Archicad's Undo takes it back one
  step at a time; use **Back to "…"** instead.
- Before each relink it writes the state of the Drawing (its view, magnification,
  angle, position and frame) to `Documents/Tapir/relink-drawing-history.json` in
  your home folder, per project, so that **Back to "…"** works. It writes no
  other file.

## Limits

- Archicad 27, 28, 29 and 30, with the Tapir Add-On 2.0.0 or later.
- Only Drawings of views of the project. Drawings of external files, schedules
  and 3D views are not supported.
- A Drawing is relinked only to a view of the same family, which shares its 2D
  coordinates: plans, worksheets and details on one side, sections and
  elevations (interior elevations included) on the other.
- Views whose transformation is mirrored or scaled are not supported.
- With the current Tapir Add-On the script works out the Drawing's source view
  from the Drawings on the layout (their scale and the drawing name in the
  title). When several fit it asks you to pick one. A later Tapir Add-On that
  returns the source view of a Drawing (pull request #2786) lets it know the
  source view directly.
- Works in metric and imperial projects: it reads and writes lengths in the
  Archicad API's units and shows only scales, angles and, for a difference it
  finds, a distance in mm and inches.

## Credits

Original by iota (Discord), shared in `#share-a-script` under the MIT licence.
Adapted for the marketplace for
[#2950](https://github.com/ENZYME-APD/tapir-archicad-automation/issues/2950).
