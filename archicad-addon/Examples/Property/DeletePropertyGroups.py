"""Creates a temporary custom property group and deletes it again with DeletePropertyGroups."""
import aclib

groupIds = aclib.RunTapirCommand ('CreatePropertyGroups', {
    'propertyGroups': [{'propertyGroup': {'name': 'Tapir Example Group To Delete'}}]
})['propertyGroupIds']

aclib.PrintJson (aclib.RunTapirCommand ('DeletePropertyGroups', {
    'propertyGroupIds': [g for g in groupIds if 'propertyGroupId' in g]
}))
