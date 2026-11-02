import aclib

elements = aclib.RunCommand ('API.GetAllElements', {})['elements'][:10]

elementIFCIds = aclib.RunTapirCommand ('GetIFCIdsOfElements', {
        'elements' : elements
    })['elementIFCIds']

# From Archicad 28 ifcId is the IFC GlobalId (22 characters). Before Archicad 28
# ifcId is the same identifier in the 36 character GUID form, and ifcGlobalId
# gives its GlobalId.
ifcGlobalIds = []
for elementIFCId in elementIFCIds:
    if 'error' in elementIFCId:
        continue
    ifcGlobalId = elementIFCId.get ('ifcGlobalId', elementIFCId['ifcId'])
    assert (len (ifcGlobalId) == 22)
    ifcGlobalIds.append (ifcGlobalId)

# GetElementsByIFCIds takes the GlobalId in every Archicad version.
elementsByIFCIds = aclib.RunTapirCommand ('GetElementsByIFCIds', {
        'ifcIds' : ifcGlobalIds
    })['elementsByIFCIds']

for elementsByIFCId in elementsByIFCIds:
    assert (len (elementsByIFCId['elements']) >= 1)
