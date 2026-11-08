"""Lists the group and name of every custom property of the project with GetAllProperties."""
import aclib

properties = aclib.RunTapirCommand ('GetAllProperties', {
    'propertyTypes': ['Custom'],
    'includeAvailability': False
})['properties']

for prop in properties:
    print (prop['propertyGroupName'], '/', prop['propertyName'], '-', prop['propertyValueType'])
