import aclib

# Requires Archicad 28 or newer.

def CheckIFCObject (ifcObject):
    assert ('hashValue' in ifcObject)
    if 'globalId' in ifcObject:
        # IFC GlobalIds are 22 characters long (base64-like encoding of a GUID).
        assert (len (ifcObject['globalId']) == 22)

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']

# The IfcProject, IfcSite and IfcBuilding objects, and the IFC story of each element.
spatialStructure = aclib.RunTapirCommand ('GetIFCSpatialStructure', {
        'elements' : elements
    })

if spatialStructure and 'error' not in spatialStructure:
    for key in ['project', 'site', 'building']:
        if key in spatialStructure:
            CheckIFCObject (spatialStructure[key])

    assert (len (spatialStructure['elements']) == len (elements))
    for elementStory in spatialStructure['elements']:
        if 'error' in elementStory:
            continue
        assert ('storyIndex' in elementStory)

# The IFC groups (e.g. IfcZone, IfcSystem) and what is assigned to what.
assignments = aclib.RunTapirCommand ('GetIFCAssignments')

if assignments and 'error' not in assignments:
    for rootGroup in assignments['rootGroups']:
        CheckIFCObject (rootGroup)
    for childObject in assignments['childObjects']:
        CheckIFCObject (childObject['object'])
        for child in childObject['children']:
            CheckIFCObject (child)
    for parentObject in assignments['parentObjects']:
        CheckIFCObject (parentObject['object'])
        for parent in parentObject['parents']:
            CheckIFCObject (parent)

# An enumerated IFC property also lists the values of the enumeration it refers to.
elementIFCProperties = aclib.RunTapirCommand ('GetIFCPropertiesOfElements', {
        'elements' : elements
    })['elementIFCProperties']

for elementIFCProperty in elementIFCProperties:
    if 'error' in elementIFCProperty:
        continue
    for ifcProperty in elementIFCProperty['ifcProperties']:
        if 'enumerationReference' in ifcProperty:
            assert (ifcProperty['propertyType'] == 'EnumeratedValue')
