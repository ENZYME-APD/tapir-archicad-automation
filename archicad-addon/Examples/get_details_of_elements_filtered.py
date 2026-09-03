import aclib

elements = aclib.RunTapirCommand ('GetAllElements', {})['elements']

# The optional 'fields' parameter selects which fields to return for each element.
# Fields not listed are not computed at all, so skipping 'floorPlanPolygons' avoids
# regenerating the 2D drawing primitives of every element - the expensive part of
# an unfiltered GetDetailsOfElements batch read.
detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': elements,
    'fields': ['type', 'layerIndex', 'details']
})['detailsOfElements']

for detailsOfElement in detailsOfElements:
    assert 'floorPlanPolygons' not in detailsOfElement
    assert 'id' not in detailsOfElement
    assert 'floorIndex' not in detailsOfElement

# Header-only read: no type-specific details and no polygon extraction at all.
detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': elements,
    'fields': ['type', 'floorIndex', 'layerIndex', 'drawIndex']
})['detailsOfElements']

for detailsOfElement in detailsOfElements:
    assert 'details' not in detailsOfElement
    assert 'floorPlanPolygons' not in detailsOfElement
