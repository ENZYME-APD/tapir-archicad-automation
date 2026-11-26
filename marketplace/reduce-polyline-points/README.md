# Reduce Polyline Points

Reduces the number of points of the selected polylines, for example contour lines
imported from a survey, while keeping them within a tolerance you choose. It can
also smooth them, and turn them into splines instead of changing them in place.
A window shows the result live before anything is changed.

## How to use it

1. Select the polylines to simplify. Other selected elements are left alone.
2. Run the script from the Tapir palette. A window opens with a preview: the
   original polylines in grey, the result in blue.
3. Move the **Reduction target** slider (the tolerance follows it), or set the
   **Tolerance** yourself (pick its unit: m, cm, mm, ft or in). The window shows
   the points before and after, the reduction in % and the largest distance
   between the original and the result (**Max deviation**).
4. Optionally set **Smoothing**, and choose the **Output**.
5. The note above the buttons says what **Apply** will change. Press **Apply**,
   or **Cancel** to change nothing.

Wheel zooms the preview, dragging pans it, double-click fits everything.

## Settings

| Setting | Default | What it does |
| --- | --- | --- |
| Reduction target | follows the tolerance | The share of points to remove, in %. The tolerance is adjusted to reach it. The largest possible reduction is shown under the slider: the ends of each polyline and the ends of its arcs are always kept. |
| Tolerance | 0.01 m | The largest distance a removed point may have from the new polyline (Douglas-Peucker simplification). |
| Smoothing factor | 0 (off) | 0 to 1: how far each point moves towards the middle of its neighbours in each iteration. Smoothing moves the points, so the max deviation can grow beyond the tolerance (it is shown in orange then). |
| Smoothing iterations | 5 | 0 to 25 smoothing passes. |
| Output | Polylines | **Polylines**: the selected polylines are changed in place (same element, layer, pen and line type). **Splines**: new splines are created with the same story, layer, pen, line type and zone boundary setting. |
| Delete the original polylines | on | Only for spline output: deletes the polylines the splines replace. |

Arc segments are kept as they are; only the straight parts between them are
simplified. For spline output the arcs are turned into points first. Archicad
smooths a spline itself, so the spline preview is approximate.

## What it changes

- **Polylines output:** the points of the selected polylines. Their other settings
  are kept. It is one Undo step.
- **Splines output:** it creates one spline per polyline, selects them, and
  deletes the original polylines unless you untick the option: two Undo steps.
- Polylines on a locked layer, or not reserved in Teamwork, are reported in the
  palette's output and left as they are.
- It writes no files.

## From the command line

Without the window, with the tolerance in meters:

```
python reduce_polyline_points.py --tol 0.02 --smooth 0.3 --iter 5
python reduce_polyline_points.py --target 80 --output spline
```

`--keep-originals` keeps the polylines with spline output, `--no-ui` uses the
defaults without showing the window, and `--port` names the Archicad to use.

## Limits

- Only Polyline elements. Lines, arcs, splines and fills are not changed.
- Archicad 25 to 30, with the Tapir Add-On 1.5.9 or later.
- Works in metric and imperial projects: the tolerance can be given in m, cm,
  mm, ft or in. On the command line `--tol` is in meters.
- The window needs Tapir's Script UI; with a very large selection (tens of
  thousands of points) the preview can be slow.

## Credits

Original by pumaka (Discord), shared in `#share-a-script` under the MIT licence.
It is the Archicad version of the Rhino script CombinedFixContourCrvs.py by Mitch
Heynick. Adapted for the marketplace for
[#3069](https://github.com/ENZYME-APD/tapir-archicad-automation/issues/3069).
