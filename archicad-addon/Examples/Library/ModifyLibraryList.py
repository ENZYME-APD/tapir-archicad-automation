"""Adds an empty temporary folder to the library list with ModifyLibraryList, then removes it again by its path.

Requires Archicad 28 or later.
"""
import tempfile

import aclib

with tempfile.TemporaryDirectory () as libraryFolder:
    aclib.RunTapirCommand ('ModifyLibraryList', {'add': [{'path': libraryFolder}]})
    aclib.RunTapirCommand ('ModifyLibraryList', {'remove': [{'path': libraryFolder}]})
