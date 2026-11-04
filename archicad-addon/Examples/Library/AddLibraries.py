"""Adds an empty temporary folder to the project's local libraries with AddLibraries.

The original list of local libraries is set back at the end with SetLibraries.
"""
import tempfile

import aclib

libraries = aclib.RunTapirCommand ('GetLibraries')['libraries']
originalLocalLibraries = [{'path': library['path']} for library in libraries if library['type'] == 'LocalLibrary']

with tempfile.TemporaryDirectory () as libraryFolder:
    aclib.PrintJson (aclib.RunTapirCommand ('AddLibraries', {'libraries': [{'path': libraryFolder}]}))
    aclib.RunTapirCommand ('SetLibraries', {'libraries': originalLocalLibraries})
