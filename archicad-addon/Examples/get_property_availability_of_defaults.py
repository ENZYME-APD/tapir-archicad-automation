import aclib

# Before creating new walls, checks which properties are available and visible in the
# current Wall tool default. A Favorite counts only after it was applied to the tool.

properties = aclib.RunCommand ('API.GetPropertyIds', {'properties': [
    {'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'},
    {'type': 'BuiltIn', 'nonLocalizedName': 'General_Width'}]})['properties']
properties = [p for p in properties if 'propertyId' in p]

result = aclib.RunTapirCommand ('GetPropertyAvailabilityOfDefaults', {
    'elementType': 'Wall',
    'properties': properties})
