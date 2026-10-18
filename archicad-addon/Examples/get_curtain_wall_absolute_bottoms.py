import aclib

curtainWalls = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'CurtainWall'
    })['elements']

subelements = aclib.RunTapirCommand (
    'GetSubelementsOfHierarchicalElements', {
        'elements': curtainWalls
    })['subelements']

# Every curtain wall followed by its panels and frames.
elements = []
for curtainWall, subelementsOfCW in zip (curtainWalls, subelements):
    elements.append (curtainWall)
    elements.extend (subelementsOfCW.get ('cWallPanels', []))
    elements.extend (subelementsOfCW.get ('cWallFrames', []))

absoluteBottoms = aclib.RunTapirCommand (
    'GetCurtainWallAbsoluteBottoms', {
        'elements': elements
    })['absoluteBottoms']

for element, absoluteBottom in zip (elements, absoluteBottoms):
    if 'error' in absoluteBottom:
        print ('{}: error: {}'.format (element['elementId']['guid'], absoluteBottom['error']['message']))
    else:
        print ('{}: absolute bottom = {:.3f}'.format (element['elementId']['guid'], absoluteBottom['absoluteBottom']))
