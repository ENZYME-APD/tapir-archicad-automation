"""Creates a Section, limits its horizontal and vertical range with SetSectionsAndElevationsDetails
and reads the settings back.

The name and the reference ID are changed with ModifySections instead. Deletes the section at the end.
"""
import aclib

aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})

section = aclib.RunTapirCommand ('CreateSections', {
    'sectionsData': [{
        'startCoordinate': {'x': 0.0, 'y': -2.0},
        'endCoordinate': {'x': 10.0, 'y': -2.0},
        'name': 'Tapir Example Section'
    }]
})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('SetSectionsAndElevationsDetails', {
    'elementsWithDetails': [{
        'elementId': section['elementId'],
        'details': {
            'horizontalRange': 'Limited',
            'verticalRange': 'Limited',
            'verticalRangeMin': 0.5,
            'verticalRangeMax': 2.5,
            'linePenIndex': 5
        }
    }]
}))

detail = aclib.RunTapirCommand ('GetSectionsAndElevationsDetails', {'elements': [section]})['detailsOfElements'][0]
aclib.PrintJson ({key: detail.get (key) for key in ('horizontalRange', 'verticalRange', 'verticalRangeMin', 'verticalRangeMax', 'linePenIndex')})

aclib.RunTapirCommand ('DeleteElements', {'elements': [section]})
