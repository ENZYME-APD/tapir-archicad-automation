import aclib

# All of these commands need Archicad 27 or later.

# The grid settings belong to the database of the current window: show the grid with a
# 1 m main spacing there, then restore the original settings.
grid = aclib.RunTapirCommand ('GetGridSettings')
aclib.RunTapirCommand ('SetGridSettings', {'settings': {'gridSwitch': True, 'spaceMainX': 1.0, 'spaceMainY': 1.0}})
aclib.RunTapirCommand ('SetGridSettings', {'settings': grid['settings']})

# Hide the walls in the 3D window, then restore the original filter.
filterAndCut = aclib.RunTapirCommand ('Get3DFilterAndCutSettings')
aclib.RunTapirCommand ('Set3DFilterAndCutSettings', {
    'settings': {'elemTypeFilter': [{'elementType': 'Wall', 'show': False}]}
})
aclib.RunTapirCommand ('Set3DFilterAndCutSettings', {'settings': filterAndCut['settings'], 'rebuild': True})

# The index and the guid of a dimension standard, found by its name.
aclib.RunTapirCommand ('FindDimensionStandard', {'name': 'Plain Meter'})

# The sun position at noon on midsummer day, then the original date and time again.
place = aclib.RunTapirCommand ('SetPlaceSettings', {'placeInfo': {}})['placeInfo']
aclib.RunTapirCommand ('SetPlaceSettings', {'placeInfo': {'month': 6, 'day': 21, 'hour': 12, 'minute': 0, 'second': 0}})
aclib.RunTapirCommand ('SetPlaceSettings', {'placeInfo': {key: place[key] for key in ['year', 'month', 'day', 'hour', 'minute', 'second', 'sumTime']}})
