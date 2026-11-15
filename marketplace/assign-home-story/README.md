# Assign Home Story

Gives every selected element the home story that matches where it sits in 3D, from its 3D
bounding box, and keeps it where it is. Useful after importing a model, after copying
elements between stories, or to clean up elements placed on the wrong story.

## How to use it

1. Select the elements in Archicad (in the 3D window you can select elements of several
   stories at once).
2. Run the script. It reads the selection and lists the elements that would change story,
   with their story now and their new story, and a summary of how many go from which story
   to which.
3. Choose how the story is picked and the snap tolerance, and press *Preview* to list them
   again. *Preview* changes nothing.
4. Press *Apply...*. The script reads the selection again, says what it will change and asks
   first.

Each story owns the height band from its level up to the level of the next story. The band
of the lowest story reaches down without end, the band of the highest story up.

From the command line (the palette passes the port of its Archicad; give `--port` yourself
otherwise):

```
AssignHomeStory.py --mode center|bottom|majority [--dry-run] [--tolerance "1 cm"]
```

With `--mode` the script runs without a window: it lists what changes and then changes it,
or with `--dry-run` only lists it.

## Settings

- *Home story by* (`--mode`), default *Centre*:
  - *Centre*: the story whose band holds the centre of the bounding box.
  - *Bottom*: the story whose band holds the bottom of the bounding box.
  - *Majority*: the story whose band holds the largest part of the bounding box's height.
    A tie goes to the lower story.
- *Snap tolerance* (`--tolerance`), default `1 cm`: a bottom or top this close to a story
  level counts as on that level, so an element starting at 2.995 m goes to the story at
  3.00 m. Write it with its unit: `mm`, `cm`, `m`, `in` (or `"`), `ft` (or `'`), e.g.
  `10 mm`, `3/8"`, `0.05 ft`. A number without a unit is in metres.

## What it changes

The home story of the listed elements, after you confirm. Changing the home story keeps an
element's offset from its story, which would move it up or down: the script reads the
bounding boxes again and moves every element that shifted back to where it was. The story
change and the move back are two Undo steps. It writes no files.

Left alone, because their story follows their host or they have no height:

- doors, windows, skylights and openings;
- the segments and parts of curtain walls, stairs, railings, beams and columns;
- 2D elements (lines, fills, texts, dimensions, labels, ...), cameras, hotlinks, drawings
  and the elements of other views.

The summary says how many of each were left alone.

## Limits

- **Tops linked to a story.** A wall or column whose top is linked to a story keeps that
  link relative to its home story, so with a new home story its top follows the new story
  and its height changes. The script does not change top links; it names every element
  whose bottom or top is not where it was before (within 1 mm) in the palette's output, and
  the status line says how many there are. Check those and set their top by hand.
- Elements that cannot be edited (hidden or locked layer, locked, or not reserved in a
  Teamwork project) are not changed; they are named in the palette's output.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- Lengths are measured in metres inside Archicad; the snap tolerance takes metric and
  imperial units.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Original by Jorge Beneitez (@JBENEITEZ), `assign_home_story.py` 1.0.0 of
[ENZYME-APD/enzyme-tapir-scripts](https://github.com/ENZYME-APD/enzyme-tapir-scripts),
shared in #2787. This version was written from the description in that issue with the Tapir
issue bot, in the marketplace format.
