"""Finds the loaded GDL objects whose name contains 'door' with FindLibraryParts.

Requires Archicad 28 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('FindLibraryParts', {
    'kind': 'Object',
    'nameContains': 'door'
}))
