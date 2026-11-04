"""Sets the project's local libraries with SetLibraries: the current ones plus an empty temporary folder.

The original list of local libraries is set back at the end. Built-in, embedded, server and web libraries are not touched.
"""
import tempfile

import aclib

libraries = aclib.RunTapirCommand ('GetLibraries')['libraries']
originalLocalLibraries = [{'path': library['path']} for library in libraries if library['type'] == 'LocalLibrary']

with tempfile.TemporaryDirectory () as libraryFolder:
    aclib.PrintJson (aclib.RunTapirCommand ('SetLibraries', {'libraries': originalLocalLibraries + [{'path': libraryFolder}]}))
    aclib.PrintJson (aclib.RunTapirCommand ('SetLibraries', {'libraries': originalLocalLibraries}))
