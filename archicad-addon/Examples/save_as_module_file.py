import os
import sys
import tempfile
import aclib

# Writes the open project as a hotlink module file, then the selected elements only.
# The module file is the source a hotlink node points at (see hotlink_instances.py).

modulePath = os.path.join (tempfile.gettempdir (), 'tapir_example.mod')

aclib.RunTapirCommand ('SaveAsModuleFile', {'moduleFilePath': modulePath})

selected = aclib.RunTapirCommand ('GetSelectedElements', {})['elements']
if len (selected) > 0:
    aclib.RunTapirCommand ('SaveAsModuleFile', {
        'moduleFilePath': os.path.join (tempfile.gettempdir (), 'tapir_example_selection.mod'),
        'elements': selected
    })
