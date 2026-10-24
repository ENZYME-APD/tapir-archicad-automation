import aclib

# SetOpeningFloorPlanParameters needs Archicad 29 or newer.
openings = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Opening' })['elements']

# Shows the reference axis of every opening with pen 5 and turns on its cover fill.
openingsWithFloorPlanParameters = []
for opening in openings:
    openingsWithFloorPlanParameters.append ({
        'elementId': opening['elementId'],
        'floorPlanParameters': {
            'showReferenceAxis': True,
            'referenceAxisPen': 5,
            'useCoverFills': True
        }
    })

aclib.RunTapirCommand ('SetOpeningFloorPlanParameters', {
        'openingsWithFloorPlanParameters': openingsWithFloorPlanParameters
    })

aclib.RunTapirCommand ('GetOpeningFloorPlanParameters', {
        'elements': openings
    })
