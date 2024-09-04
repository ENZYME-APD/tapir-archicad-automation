import aclib

mainElements = aclib.RunCommand ('API.GetAllElements', {})['elements']
response = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {
        'hierarchicalElements' : mainElements
    })

allSubElements = []
for subelementGroups in response['subelementsOfHierarchicalElements']:
    for subelements in subelementGroups.values():
        for subelement in subelements:
            if 'elementId' in subelement:
                allSubElements.append (subelement)

classificationSystems = [{'classificationSystemId': classificationSystem['classificationSystemId']} for classificationSystem in aclib.RunCommand ('API.GetAllClassificationSystems', {})['classificationSystems']]

classificationsOfMainElements = aclib.RunTapirCommand ('GetClassificationsOfElements', {
        'elements' : mainElements,
        'classificationSystemIds' : classificationSystems
    })

classificationsOfSubElements = aclib.RunTapirCommand ('GetClassificationsOfElements', {
        'elements' : allSubElements,
        'classificationSystemIds' : classificationSystems
    })