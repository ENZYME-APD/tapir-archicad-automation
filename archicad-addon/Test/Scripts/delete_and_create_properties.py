import aclib

# Get all details of all properties and groups

propertyIds = aclib.RunCommand ('API.GetAllPropertyIds', {'propertyType': 'UserDefined'})['propertyIds']
propertyDefinitions = aclib.RunCommand ('API.GetDetailsOfProperties', {'properties': propertyIds})['propertyDefinitions']
propertyDefinitionAvailabilityList = aclib.RunCommand ('API.GetPropertyDefinitionAvailability', {'propertyIds': propertyIds})['propertyDefinitionAvailabilityList']
propertyGroupIds = aclib.RunCommand ('API.GetAllPropertyGroupIds', {'propertyType': 'UserDefined'})['propertyGroupIds']
propertyGroups = aclib.RunCommand ('API.GetPropertyGroups', {'propertyGroupIds': propertyGroupIds})['propertyGroups']

# Delete all properties and groups

aclib.RunTapirCommand ('DeletePropertyDefinitions', {'propertyIds': propertyIds})
aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': propertyGroupIds})

propertyIds = aclib.RunCommand ('API.GetAllPropertyIds', {'propertyType': 'UserDefined'})['propertyIds']
if propertyIds:
    print ('FAILED to delete all custom properties')
    exit (1)
propertyGroupIds = aclib.RunCommand ('API.GetAllPropertyGroupIds', {'propertyType': 'UserDefined'})['propertyGroupIds']
if propertyGroupIds:
    print ('FAILED to delete all custom property groups')
    exit (1)

# ReCreate all properties and groups

for i in range(len(propertyDefinitions)):
    del propertyDefinitions[i]['propertyDefinition']['propertyId']
    del propertyDefinitions[i]['propertyDefinition']['group']['propertyGroupId']
    propertyDefinitions[i]['propertyDefinition']['availability'] = propertyDefinitionAvailabilityList[i]['propertyDefinitionAvailability']['availableClassifications']
for propertyGroup in propertyGroups:
    del propertyGroup['propertyGroup']['propertyGroupId']

aclib.RunTapirCommand ('CreatePropertyGroups', {'propertyGroups': propertyGroups})
aclib.RunTapirCommand ('CreatePropertyDefinitions', {'propertyDefinitions': propertyDefinitions})

# Get all details of all properties and groups

propertyIds = aclib.RunCommand ('API.GetAllPropertyIds', {'propertyType': 'UserDefined'})['propertyIds']
propertyDefinitionsAfterRecreate = aclib.RunCommand ('API.GetDetailsOfProperties', {'properties': propertyIds})['propertyDefinitions']
propertyDefinitionAvailabilityListAfterRecreate = aclib.RunCommand ('API.GetPropertyDefinitionAvailability', {'propertyIds': propertyIds})['propertyDefinitionAvailabilityList']
propertyGroupIds = aclib.RunCommand ('API.GetAllPropertyGroupIds', {'propertyType': 'UserDefined'})['propertyGroupIds']
propertyGroupsAfterRecreate = aclib.RunCommand ('API.GetPropertyGroups', {'propertyGroupIds': propertyGroupIds})['propertyGroups']

# Compare current state to the state before deleted all

for i in range(len(propertyGroupsAfterRecreate)):
    del propertyGroupsAfterRecreate[i]['propertyGroup']['propertyGroupId']

    beforeDeleteStr  = aclib.JsonDumpDictionary (propertyGroups[i])
    afterRecreateStr = aclib.JsonDumpDictionary (propertyGroupsAfterRecreate[i])
    if beforeDeleteStr != afterRecreateStr:
        print ('FAILED')
        print (beforeDeleteStr)
        print (afterRecreateStr)
        exit (1)

for i in range(len(propertyDefinitionsAfterRecreate)):
    del propertyDefinitionsAfterRecreate[i]['propertyDefinition']['propertyId']
    del propertyDefinitionsAfterRecreate[i]['propertyDefinition']['group']['propertyGroupId']
    propertyDefinitionsAfterRecreate[i]['propertyDefinition']['availability'] = propertyDefinitionAvailabilityListAfterRecreate[i]['propertyDefinitionAvailability']['availableClassifications']

    beforeDeleteStr  = aclib.JsonDumpDictionary (propertyDefinitions[i])
    afterRecreateStr = aclib.JsonDumpDictionary (propertyDefinitionsAfterRecreate[i])
    if beforeDeleteStr != afterRecreateStr:
        print ('FAILED')
        print (beforeDeleteStr)
        print (afterRecreateStr)
        exit (1)

if len(propertyGroups) != len(propertyGroupsAfterRecreate):
    print ('FAILED to create some property groups')
    exit (1)
if len(propertyDefinitions) != len(propertyDefinitionsAfterRecreate):
    print ('FAILED to create some properties')
    exit (1)

print ('SUCCEEDED')