import aclib

# GetDetailsOfElements reports two heights of a curtain wall: 'height', the height setting stored
# on the element, and 'actualHeight', the height Archicad calculates (ACAPI_CurtainWall_GetCWHeight).

allCurtainWalls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'CurtainWall'
    })['elements']

detailsOfCurtainWalls = aclib.RunTapirCommand (
    'GetDetailsOfElements', {
        'elements': allCurtainWalls
    })['detailsOfElements']

for curtainWall, details in zip (allCurtainWalls, detailsOfCurtainWalls):
    typeSpecificDetails = details.get ('details', {})
    print ('{}: height {}, actualHeight {}'.format (
        curtainWall['elementId']['guid'], typeSpecificDetails.get ('height'), typeSpecificDetails.get ('actualHeight')))
