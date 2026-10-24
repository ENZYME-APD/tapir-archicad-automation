import aclib

# GetOpeningFloorPlanParameters and GetOpeningDefault need Archicad 29 or newer.
openings = aclib.RunCommand ('API.GetElementsByType', { 'elementType': 'Opening' })['elements']

floorPlanParameters = aclib.RunTapirCommand ('GetOpeningFloorPlanParameters', {
        'elements': openings
    })['openings']

openingDefault = aclib.RunTapirCommand ('GetOpeningDefault', {})
