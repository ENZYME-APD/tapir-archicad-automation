import aclib

# The Partial Structure Display of the current (front) window, e.g. the floor plan or the 3D window.
# Both commands are available from Archicad 27; on older versions they return an error.
current = aclib.RunTapirCommand ('GetStructureDisplay')

if current is not None and 'error' not in current:
    # Show only the core of the structures, e.g. before exporting an image ...
    aclib.RunTapirCommand ('SetStructureDisplay', {
        'structureDisplay': 'CoreOnly'
    })
    aclib.RunTapirCommand ('GetStructureDisplay')

    # ... then restore what the user saw before.
    aclib.RunTapirCommand ('SetStructureDisplay', {
        'structureDisplay': current['structureDisplay']
    })
