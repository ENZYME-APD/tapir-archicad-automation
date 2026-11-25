"""Merges a module file into the project with MergeFile, 10 m north of where it was saved.

The script saves a wall it creates as a module in a temporary folder first; the wall and the merged
elements are deleted at the end. Requires Archicad 27 or later (SaveAsModuleFile with an element list).
"""
import os
import tempfile

import aclib

walls = aclib.RunTapirCommand ('CreateWalls', {'wallsData': [
    {'begCoordinate': {'x': 100.0, 'y': 100.0}, 'endCoordinate': {'x': 104.0, 'y': 100.0}, 'height': 3.0, 'floorIndex': 0}
]})['elements']
modulePath = os.path.join (tempfile.mkdtemp (), 'TapirMergeFileExample.mod')
aclib.RunTapirCommand ('SaveAsModuleFile', {'moduleFilePath': modulePath, 'elements': walls})

# The file's origin lands at (0, 10): the merged wall runs from (100, 110) to (104, 110).
merged = aclib.RunTapirCommand ('MergeFile', {'sourceLocation': modulePath, 'origin': {'x': 0.0, 'y': 10.0}})
aclib.PrintJson (merged)

aclib.RunTapirCommand ('DeleteElements', {'elements': walls + merged['elements']})
os.remove (modulePath)
