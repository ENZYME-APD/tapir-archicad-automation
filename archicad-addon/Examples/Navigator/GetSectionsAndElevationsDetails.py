"""Creates a Section and reads its settings back with GetSectionsAndElevationsDetails.

Prints its name, reference ID and ranges, then deletes the section.
"""
import aclib

aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})

section = aclib.RunTapirCommand ('CreateSections', {
    'sectionsData': [{
        'startCoordinate': {'x': 0.0, 'y': -2.0},
        'endCoordinate': {'x': 10.0, 'y': -2.0},
        'name': 'Tapir Example Section',
        'id': 'TX-1'
    }]
})['elements'][0]

details = aclib.RunTapirCommand ('GetSectionsAndElevationsDetails', {'elements': [section]})['detailsOfElements']
for detail in details:
    aclib.PrintJson ({key: detail.get (key) for key in ('name', 'id', 'horizontalRange', 'verticalRange')})

aclib.RunTapirCommand ('DeleteElements', {'elements': [section]})
