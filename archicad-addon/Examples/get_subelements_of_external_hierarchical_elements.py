import aclib

# External hierarchical elements (for example MEP elements or elements of other add-ons)
# return their subelements as externalSubelements from Archicad 28.
mainElements = aclib.RunCommand ('API.GetAllElements', {})['elements']
response = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {
        'elements' : mainElements
    })

for mainElement, subelementGroups in zip (mainElements, response['subelements']):
    if 'externalSubelements' not in subelementGroups:
        continue
    print ('{} has {} external subelements:'.format (mainElement['elementId']['guid'], len (subelementGroups['externalSubelements'])))
    for subelement in subelementGroups['externalSubelements']:
        print ('    {}'.format (subelement['elementId']['guid']))
