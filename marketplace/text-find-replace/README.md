# Find and Replace in Texts

Finds a word or phrase in the content of Texts and text Labels and replaces it in the matches
you check. The styles of the rest of the text and its autotexts stay as they are.

## How to use it

1. Run the script, type the text to find in *Find* and the new text in *Replace with*.
2. Choose where to search (*Selection*, *Current story* or *All stories*) and, if you want,
   *Match case* and *Whole word*. Press *Find*.
3. Every match is listed with its story, its element and a preview of the line before and after
   the replace. Click the box in the *Replace* column (or press the space bar) to check or
   uncheck a match; *Check all* and *Uncheck all* do it for every match.
4. *Select in Archicad* selects the elements of the selected rows, or of the checked matches.
5. Press *Replace checked* and confirm. The script searches again afterwards, so the list shows
   what is left.

## Settings

- *Where*: *All stories*.
- *Match case*: off. *Whole word*: off.

## What it changes

The content of the Texts and text Labels with checked matches, after you confirm. You can undo
it in Archicad. An element whose text changed since the search is left alone and counted. It
writes no files.

## Limits

- The text to find and the replacement are each one line.
- A match inside an autotext, or after an autotext whose value differs from its key, is listed
  but cannot be checked: change it in Archicad.
- Labels that are not text labels (symbol labels) are not searched.
- At most 5000 matches are listed at a time; search again after replacing them.
- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- In a Teamwork project, an element you have not reserved may not be changed; the first error
  is shown in the status line.
- It needs the `utilities/` and `aclib/` folders of
  [`builtin-scripts/`](../../builtin-scripts/) next to it (see
  [The helpers `utilities` and `aclib`](../README.md#the-helpers-utilities-and-aclib)).

## Credits

Requested by @parikshit99 in #2002, written with the Tapir issue bot.
