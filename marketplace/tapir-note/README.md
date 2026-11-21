# Tapir Note

Writes down the steps of a task that someone has to do by hand, because no script can do them, and
hands them to a team member as an Archicad Issue. The issue is named after the task, tagged with the
person it is for, has one comment per step, and has the selected elements attached, so they find
everything in the Issue Manager.

## How to use it

1. Select the elements the task is about (optional).
2. Run the script. Fill in:
   - *Task*: the issue's name, e.g. `Fix the door labels on level 2`.
   - *For*: who should do it. The issue's tag becomes `For <name>`, or `Tapir Note` when it is empty.
   - *From*: your name, written as the author of the comments. It starts with your computer's user name.
   - *Steps to do by hand*: one step per line. A `1.`, `2)`, `-` or `•` you type in front of a line
     is left out, because the script numbers the steps itself.
3. Press *Create note*. The script creates the issue, adds a first comment with who it is for, who
   wrote it and the date, then one comment per step (`Step 1 of 3: ...`), and attaches the selected
   elements as *Highlight*.
4. To share it:
   - In a Teamwork project, *Send* your changes. The issue is then in everyone's Issue Manager.
   - For someone outside the project, press *Export as BCF...* and send them the `.bcfzip` file.
     It holds only this issue.

## Settings

- *Attach the selected elements*: on.

## What it changes

*Create note* creates one Issue with its comments and attaches the selected elements to it. It
changes nothing else in the model. *Export as BCF...* writes one `.bcfzip` file, where you choose in
the save dialog. Its default name is the task's name.

## Limits

- Works in Archicad 25 to 30 with Tapir 1.8.0 or later.
- The steps are comments of the issue. Ticking off a step means adding a comment in the Issue Manager:
  Archicad issues have no checklist.
- The script doesn't send messages or e-mails. The note reaches a team member through Teamwork or a
  BCF file.

## Credits

Requested by Abdelaziz Nacer on the Tapir Discord (#2796), written with the Tapir issue bot.
