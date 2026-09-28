import aclib

# Renames a property group this script creates, then removes it. The group keeps its
# identifier, so the properties inside it stay where they are.

created = aclib.RunTapirCommand ('CreatePropertyGroups', {'propertyGroups': [
    {'propertyGroup': {'name': 'Tapir Example Group Before'}}]}, debug = False)
groupId = created['propertyGroupIds'][0]['propertyGroupId']

result = aclib.RunTapirCommand ('UpdatePropertyGroups', {'propertyGroups': [
    {'propertyGroupId': groupId, 'name': 'Tapir Example Group After', 'description': 'renamed'}]}, debug = False)
print ('Rename succeeded: {}'.format (result['executionResults'][0]['success']))

groups = aclib.RunTapirCommand ('GetAllProperties', {}, debug = False).get ('propertyGroups', [])
print ('Found under the new name: {}'.format (any (g['name'] == 'Tapir Example Group After' for g in groups)))

aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [{'propertyGroupId': groupId}]}, debug = False)
