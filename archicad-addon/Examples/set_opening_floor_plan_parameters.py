import aclib

# SetOpeningFloorPlanParameters needs Archicad 29 or newer.
openings = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Opening' })['elements']

# Shows every opening symbolic and cut on the floor plan, with its reference axis in pen 5 and its cover fill on.
openingsWithFloorPlanParameters = []
for opening in openings:
    openingsWithFloorPlanParameters.append ({
        'elementId': opening['elementId'],
        'floorPlanParameters': {
            'floorPlanDisplayMode': 'SymbolicCut',
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
