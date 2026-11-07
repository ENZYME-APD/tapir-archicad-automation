"""Prints how many bodies, polygons, edges, vertices etc. the current 3D model has, with Get3DModelCounts.

Requires Archicad 27 or later.
"""
import aclib

counts = aclib.RunTapirCommand ('Get3DModelCounts', {})['counts']
for item in counts:
    print ('{}: {}'.format (item.get ('componentType'), item.get ('count')))
