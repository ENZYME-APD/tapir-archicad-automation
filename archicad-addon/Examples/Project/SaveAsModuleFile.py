"""Saves the first ten walls as a hotlink module (.mod) in the temp folder with SaveAsModuleFile.

Needs at least one Wall. Without elements the current selection is saved.
"""
import os
import tempfile
import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements'][:10]
if not walls:
    raise SystemExit ('No wall in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('SaveAsModuleFile', {
    'moduleFilePath': os.path.join (tempfile.mkdtemp (), 'TapirWalls.mod'),
    'elements': walls
}))
