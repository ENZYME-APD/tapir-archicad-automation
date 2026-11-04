"""Creates a new .gsm file of the 'General GDL Object' subtype with CreateLibraryPart.

The file is created in a temporary folder that is added to the library list for the time of the script, so the new
part is loaded; the folder is removed from the list at the end. Requires Archicad 28 or later.
"""
import os
import tempfile

import aclib

with tempfile.TemporaryDirectory () as libraryFolder:
    aclib.RunTapirCommand ('ModifyLibraryList', {'add': [{'path': libraryFolder}]})

    aclib.PrintJson (aclib.RunTapirCommand ('CreateLibraryPart', {
        'path': os.path.join (libraryFolder, 'Tapir New Object.gsm'),
        'ancestorSubtype': 'General GDL Object.gsm'
    }))

    aclib.RunTapirCommand ('ModifyLibraryList', {'remove': [{'path': libraryFolder}]})
