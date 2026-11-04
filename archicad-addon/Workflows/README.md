# Workflows

Scripts that combine several Tapir commands into a task or a tool, e.g. auditing the library
parts or building materials of a project, syncing attributes between two Archicads
(`sync_attributes_gui.py`), clipping drawings to the zones (`zone_clip.py`) or creating one of every
element type (`create_elements.py`). For one command at a time, see the
[examples](../Examples/).

Run them like the examples, with Archicad and the Tapir Add-On running:

```
python archicad-addon/Workflows/create_elements.py
```

Many of them change the open project, so try them on a copy first; read the comment at the top
of each script. `aclib.py` here only loads [`../Examples/aclib/`](../Examples/aclib/), the helper
the scripts import.
