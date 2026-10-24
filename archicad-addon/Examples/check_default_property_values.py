import aclib

# Before people draw new walls, checks the property values the current Wall tool default
# gives them. A Favorite counts only after it was applied to the tool.

properties = aclib.RunCommand ('API.GetPropertyIds', {'properties': [
    {'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'},
    {'type': 'BuiltIn', 'nonLocalizedName': 'General_Width'}]})['properties']
properties = [p for p in properties if 'propertyId' in p]

result = aclib.RunTapirCommand ('GetPropertyValuesOfElementDefaults', {
    'elementType': 'Wall',
    'properties': properties})
