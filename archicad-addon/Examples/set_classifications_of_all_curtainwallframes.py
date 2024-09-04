import json
import aclib

allCurtainWalls = aclib.RunCommand (
    'API.GetElementsByType', {
        'elementType': 'CurtainWall'
    })['elements']
print('Project contains {} Curtain Wall(s)'.format(len(allCurtainWalls)))

allCWSubelements = aclib.RunTapirCommand (
    'GetSubelementsOfHierarchicalElements', {
        'hierarchicalElements': allCurtainWalls
    }, debug=False)['subelementsOfHierarchicalElements']

allCWFrameSubelements = [subelement for subelements in allCWSubelements for subelement in subelements['cWallFrames']]
print('Project contains {} Curtain Wall Frame(s)'.format(len(allCWFrameSubelements)))

classificationSystems = aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']
print('Project contains {} Classification System(s)'.format(len(classificationSystems)))

classificationSystemGuidToName = {s['classificationSystemId']['guid']: s["name"] for s in classificationSystems}
classificationItemGuidToName = dict()
classificationsOfAllCWFrameSubelements = aclib.RunTapirCommand ('GetClassificationsOfElements', {
        'elements' : allCWFrameSubelements,
        'classificationSystemIds' : [{'classificationSystemId': s['classificationSystemId']} for s in classificationSystems]
    }, debug=False)['elementClassifications']

classificationIdCounter = dict()

for i in range(len(allCWFrameSubelements)):
    cwFrameSubelement = allCWFrameSubelements[i]
    classificationIds = classificationsOfAllCWFrameSubelements[i]
    for classificationId in classificationIds['classificationIds']:
        systemGuid = classificationId['classificationSystemId']['guid']
        itemGuid = classificationId['classificationItemId']['guid']
        if systemGuid not in classificationIdCounter:
            classificationIdCounter[systemGuid] = dict()
        if itemGuid not in classificationIdCounter[systemGuid]:
            classificationIdCounter[systemGuid][itemGuid] = [cwFrameSubelement]
        else:
            classificationIdCounter[systemGuid][itemGuid].append(cwFrameSubelement)
        if itemGuid not in classificationItemGuidToName:
            classificationItemDetails = aclib.RunCommand ('API.GetDetailsOfClassificationItems', {'classificationItemIds': [{'classificationItemId': {'guid': itemGuid}}]})['classificationItems'][0]
            if 'classificationItem' in classificationItemDetails and 'id' in classificationItemDetails['classificationItem']:
                classificationItemGuidToName[itemGuid] = classificationItemDetails['classificationItem']['id']

for classificationSystemGuid, classificationItemGuidsOfElements in classificationIdCounter.items():
    print('Curtain Wall Frames in Classification System "{}" have {} different Classification Items'.format(classificationSystemGuidToName[classificationSystemGuid], len(classificationItemGuidsOfElements)))
    sortedItems = sorted(classificationItemGuidsOfElements.items(), key=lambda item: len(item[1]), reverse=True)
    for classificationItemGuid, cwFrameSubelements in sortedItems:
        print('\t{} Curtain Wall Frame(s) have Classification Item "{}"'.format(len(cwFrameSubelements), classificationItemGuidToName[classificationItemGuid]))
    classificationItemGuidWithTheHighestCounter = sortedItems[0][0]
    cwFrameSubelementsWhichAreNotUsingThatClassificationItemGuid = []
    for classificationItemGuid, cwFrameSubelements in sortedItems:
        if classificationItemGuid != classificationItemGuidWithTheHighestCounter:
            cwFrameSubelementsWhichAreNotUsingThatClassificationItemGuid += cwFrameSubelements
    if len(cwFrameSubelementsWhichAreNotUsingThatClassificationItemGuid) > 0:
        print('\tSet {} Curtain Wall Frame(s) to "{}" in Classification System "{}" ...'.format(len(cwFrameSubelementsWhichAreNotUsingThatClassificationItemGuid), classificationItemGuidToName[classificationItemGuidWithTheHighestCounter], classificationSystemGuidToName[classificationSystemGuid]))
        response = aclib.RunTapirCommand ('SetClassificationsOfElements', {
                'elementClassifications' : [{
                    'elementId': cwFrame['elementId'],
                    'classificationId': {
                        'classificationSystemId': {'guid': classificationSystemGuid},
                        'classificationItemId': {'guid': classificationItemGuidWithTheHighestCounter}
                    }
                } for cwFrame in cwFrameSubelementsWhichAreNotUsingThatClassificationItemGuid]
            }, debug=False)
        allSucceeded = all(executionResult['success'] for executionResult in response['executionResults'])
        if allSucceeded:
            print('\tAll succeeded')
        else:
            print('\tFailed to set all')
            print('Response:\n' + json.dumps (response, indent = 4))