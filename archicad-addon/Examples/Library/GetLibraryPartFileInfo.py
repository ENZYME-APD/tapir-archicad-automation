"""Reads the unique IDs and the master, 2D and 3D scripts of a library part file with GetLibraryPartFileInfo.

Uses the file of the first loaded GDL object whose name contains 'door'; the file is read
without loading it into the project. Requires Archicad 28 or later.
"""
import aclib

libraryParts = aclib.RunTapirCommand ('FindLibraryParts', {
    'kind': 'Object',
    'nameContains': 'door',
    'firstOnly': True
})['libraryParts']
if not libraryParts or 'path' not in libraryParts[0]:
    raise SystemExit ('No door library part file is loaded.')

aclib.PrintJson (aclib.RunTapirCommand ('GetLibraryPartFileInfo', {
    'files': [libraryParts[0]['path']],
    'scripts': ['master', '2d', '3d']
}))
