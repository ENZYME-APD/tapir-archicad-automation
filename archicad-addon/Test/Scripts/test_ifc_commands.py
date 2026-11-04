import aclib

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']

ifcIds = aclib.RunTapirCommand ('GetIFCIdsOfElements', {
        'elements' : elements
    })['elementIFCIds']

elementsByIFCIds = aclib.RunTapirCommand ('GetElementsByIFCIds', {
        'ifcIds' : [ifcId['ifcId'] for ifcId in ifcIds]
    })['elementsByIFCIds']

elementIFCTypes = aclib.RunTapirCommand ('GetIFCTypeOfElements', {
        'elements' : elements
    })['elementIFCTypes']

elementIFCProperties = aclib.RunTapirCommand ('GetIFCPropertiesOfElements', {
        'elements' : elements
    })['elementIFCProperties']