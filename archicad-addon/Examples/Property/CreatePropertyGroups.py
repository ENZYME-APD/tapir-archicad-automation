"""Creates a custom property group with CreatePropertyGroups, then deletes it."""
import aclib

groupIds = aclib.RunTapirCommand ('CreatePropertyGroups', {
    'propertyGroups': [{'propertyGroup': {'name': 'Tapir Example Group', 'description': 'Created by a Tapir example'}}]
})['propertyGroupIds']

aclib.RunTapirCommand ('DeletePropertyGroups', {'propertyGroupIds': [g for g in groupIds if 'propertyGroupId' in g]}, debug = False)
