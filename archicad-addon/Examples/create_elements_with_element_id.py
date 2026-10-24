"""
Sets the Element ID (the ID field of the Settings dialog, shown in schedules and
labels) through Tapir: optional 'id' on the items of the Create* commands and on
the details of SetDetailsOfElements. GetDetailsOfElements returns the same string
as 'id', so a round trip is identical.

Items without 'id' keep the next automatic ID of their tool, as before.
"""

import aclib

# --- create with an Element ID -----------------------------------------------

walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [
        {
            'id': 'AW-001',
            'begCoordinate': { 'x': 0.0, 'y': 0.0 },
            'endCoordinate': { 'x': 8.0, 'y': 0.0 },
            'height': 3.0,
            'thickness': 0.3
        },
        {
            # no 'id': this one gets the automatic ID of the Wall tool
            'begCoordinate': { 'x': 0.0, 'y': 4.0 },
            'endCoordinate': { 'x': 8.0, 'y': 4.0 },
            'height': 3.0,
            'thickness': 0.3
        }
    ]
})['elements']

columns = aclib.RunTapirCommand ('CreateColumns', {
    'columnsData': [
        { 'id': 'C-01', 'coordinates': { 'x': 10.0, 'y': 0.0, 'z': 0.0 } }
    ]
})['elements']

createdElements = [e for e in walls + columns if 'elementId' in e]

details = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': createdElements,
    'fields': ['type', 'id']
})['detailsOfElements']
# AW-001, the automatic wall ID, C-01

# --- change the Element ID of existing elements --------------------------------

aclib.RunTapirCommand ('SetDetailsOfElements', {
    'elementsWithDetails': [
        {
            'elementId': walls[0]['elementId'],
            'details': { 'id': 'AW-002' }
        },
        {
            'elementId': walls[1]['elementId'],
            # other details can be changed in the same call
            'details': { 'id': 'AW-003', 'layerIndex': 1 }
        }
    ]
})

details = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': walls,
    'fields': ['id']
})['detailsOfElements']
# AW-002, AW-003
