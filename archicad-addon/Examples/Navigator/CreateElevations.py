"""Creates an Elevation on the floor plan with CreateElevations, then deletes it again.

The optional 'details' block takes the settings of SetSectionsAndElevationsDetails. The floor plan
must be the active window, so the script switches to it first.
"""
import aclib

aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})

elevations = aclib.RunTapirCommand ('CreateElevations', {
    'elevationsData': [{
        'startCoordinate': {'x': 0.0, 'y': -5.0},
        'endCoordinate': {'x': 10.0, 'y': -5.0},
        'depth': 20.0,
        'name': 'Tapir Example Elevation',
        'id': 'TE-1',
        'details': {'horizontalRange': 'Limited', 'linePenIndex': 5}
    }]
})['elements']
aclib.PrintJson (elevations)

aclib.RunTapirCommand ('DeleteElements', {'elements': [e for e in elevations if 'elementId' in e]})
