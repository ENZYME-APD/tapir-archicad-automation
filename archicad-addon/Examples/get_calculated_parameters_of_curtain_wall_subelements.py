import aclib

# The frames, panels, junctions and accessories of a curtain wall are GDL objects. Archicad
# calculates some of their parameters for their place in the curtain wall (e.g. the size of
# a panel). GetCalculatedParametersOfCurtainWallSubElements returns those calculated values,
# GetGDLParametersOfElements the parameters the sub-element was set up with.

curtainWalls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'CurtainWall'})['elements']

if len (curtainWalls) == 0:
    print ('No curtain walls in the project.')
else:
    subelementsOfCurtainWalls = aclib.RunTapirCommand (
        'GetSubelementsOfHierarchicalElements', {'elements': curtainWalls})['subelements']

    subelements = []
    for subelementsOfCurtainWall in subelementsOfCurtainWalls:
        for subelementType in ['cWallFrames', 'cWallPanels', 'cWallJunctions', 'cWallAccessories']:
            subelements += subelementsOfCurtainWall.get (subelementType, [])[:1]

    if len (subelements) > 0:
        aclib.RunTapirCommand (
            'GetCalculatedParametersOfCurtainWallSubElements', {'elements': subelements})
