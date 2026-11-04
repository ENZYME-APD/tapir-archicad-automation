import aclib

# GetModelExtent returns the region Fit in Window would show in the current database,
# vertical range included. A stray element far from the building makes it much larger
# than expected.
extent = aclib.RunTapirCommand ('GetModelExtent')

if extent is None or 'error' in extent:
    print ('Failed to get the model extent.')
    raise SystemExit (1)

width = extent['xMax'] - extent['xMin']
depth = extent['yMax'] - extent['yMin']
height = extent['zMax'] - extent['zMin']
print ('x: {0:.2f} .. {1:.2f}'.format (extent['xMin'], extent['xMax']))
print ('y: {0:.2f} .. {1:.2f}'.format (extent['yMin'], extent['yMax']))
print ('z: {0:.2f} .. {1:.2f}'.format (extent['zMin'], extent['zMax']))
print ('The model spans {0:.2f} x {1:.2f} x {2:.2f} m'.format (width, depth, height))
