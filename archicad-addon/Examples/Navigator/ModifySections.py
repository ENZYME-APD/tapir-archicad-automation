"""Creates a Section, changes its name and reference ID with ModifySections and reads them back.

Deletes the section at the end.
"""
import aclib

section = aclib.RunTapirCommand ('CreateSections', {
    'sectionsData': [{
        'startCoordinate': {'x': 0.0, 'y': -2.0},
        'endCoordinate': {'x': 10.0, 'y': -2.0},
        'name': 'Tapir Example Section',
        'id': 'TX-1'
    }]
})['elements'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifySections', {
    'sectionsWithDetails': [{'elementId': section['elementId'], 'name': 'Tapir Renamed Section', 'id': 'TX-2'}]
}))
aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [section]})

aclib.RunTapirCommand ('DeleteElements', {'elements': [section]})
