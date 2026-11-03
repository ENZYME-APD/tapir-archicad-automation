import aclib

curtainWalls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'CurtainWall'})['elements']
subelementsResponse = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {
        'elements' : curtainWalls
    })

cWallFrames = []
for subelementGroups in subelementsResponse['subelements']:
    cWallFrames.extend (subelementGroups.get ('cWallFrames', []))

rootOwners = aclib.RunTapirCommand ('GetHierarchicalElementOwners', {
        'elements' : cWallFrames,
        'ownerMode' : 'root'
    })

parentOwners = aclib.RunTapirCommand ('GetHierarchicalElementOwners', {
        'elements' : cWallFrames,
        'ownerMode' : 'parent'
    })

ownersOfMainElements = aclib.RunTapirCommand ('GetHierarchicalElementOwners', {
        'elements' : curtainWalls
    })
