"""Creates an Interior Elevation of two segments, shows it on one story only, renames its second
segment and limits that segment's vertical range with SetInteriorElevationsDetails, then reads the
settings back with GetDetailsOfElements.

A segment is chosen by its index in the segments GetDetailsOfElements returns. Deletes the interior
elevation at the end.
"""
import aclib

aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})

interiorElevation = aclib.RunTapirCommand ('CreateInteriorElevations', {
    'interiorElevationsData': [{
        'nodeCoordinates': [{'x': 0.0, 'y': -2.0}, {'x': 6.0, 'y': -2.0}, {'x': 6.0, 'y': 2.0}],
        'depth': 1.0,
        'name': 'Tapir Example Interior Elevation'
    }]
})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('SetInteriorElevationsDetails', {
    'interiorElevationsWithDetails': [{
        'elementId': interiorElevation['elementId'],
        'details': {
            'showOnStories': 'OneStory',
            'storyToShowOn': 0,
            'segments': [{
                'segmentIndex': 1,
                'name': 'Tapir Example East Wall',
                'id': 'IE-E',
                'settings': {
                    'verticalRange': 'Limited',
                    'verticalRangeMin': 0.0,
                    'verticalRangeMax': 3.0
                }
            }]
        }
    }]
}))

details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [interiorElevation]})['detailsOfElements'][0]['details']
aclib.PrintJson ({
    'showOnStories': details.get ('showOnStories'),
    'segments': [{
        'name': segment['name'],
        'idStr': segment['idStr'],
        'verticalRange': segment['settings'].get ('verticalRange'),
        'verticalRangeMax': segment['settings'].get ('verticalRangeMax')
    } for segment in details['segments']]
})

aclib.RunTapirCommand ('DeleteElements', {'elements': [interiorElevation]})
