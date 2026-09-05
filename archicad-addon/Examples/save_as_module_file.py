import os
import tempfile
import aclib

# Writes every element of the open project as a hotlink module file. Without an element
# list the command saves the current selection, as Save Selection as Module does.

elements = aclib.RunTapirCommand ('GetAllElements', {})['elements']

if len (elements) == 0:
    print ('Nothing to save - the project has no elements.')
else:
    aclib.RunTapirCommand ('SaveAsModuleFile', {
        'moduleFilePath': os.path.join (tempfile.gettempdir (), 'tapir_example.mod'),
        'elements': elements
    })
