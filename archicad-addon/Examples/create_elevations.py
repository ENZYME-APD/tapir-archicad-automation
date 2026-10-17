import aclib

# Creates an elevation on the floor plan, looking at the line from start to end.
# The optional 'details' block takes the same settings as SetSectionsAndElevationsDetails.
aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})

newElevations = aclib.RunTapirCommand ('CreateElevations', {
    'elevationsData': [
        {
            'startCoordinate': {'x': 0.0, 'y': -5.0},
            'endCoordinate': {'x': 10.0, 'y': -5.0},
            'depth': 20.0,
            'name': 'South Elevation',
            'details': {'referenceId': 'E-01', 'linePenIndex': 5}
        }
    ]
})
print (newElevations)
