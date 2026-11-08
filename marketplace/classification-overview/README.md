# Classification Overview

Shows how many elements are classified with each item of a classification system, as a tree
that follows the system's hierarchy, and how many elements have no classification in it.

## How to use it

1. Open the project and run the script. It reads the project's classification systems and
   counts the elements of the first one.
2. Pick another system in the *Classification system* list to count its elements.
3. Each row shows the elements classified with that item (*Elements*) and with it or any item
   below it (*With sub-items*). The last row, *(not classified in this system)*, holds the
   elements without a classification in the system.
4. Select rows to highlight their elements in Archicad (the others are dimmed). Double-click a
   row, or select rows and press *Select in Archicad*, to select their elements.
5. *Export...* saves the counts as an Excel workbook (`.xlsx`) or a CSV file.

## Settings

- *Classification system*: the first system of the project at the start.
- *Hide items without elements*: on.

## What it changes

Nothing in the model. It highlights elements while rows are selected (the highlight is cleared
when you close the window) and changes the selection when you ask it to.

*Export...* writes one file, where you choose in the save dialog; its default name is
`Classification <date> <time>.xlsx`, with the columns ID, Name, Parent ID, Elements and With
sub-items.

## Limits

- All the elements of the project are counted, on every story.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later. Archicad 25 does not dim the elements
  that are not highlighted.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #1992, written with the Tapir issue bot, based on the
`get_elements_by_classification.py` workflow example.
