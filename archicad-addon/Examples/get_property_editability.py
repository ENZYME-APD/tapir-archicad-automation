import aclib

# Before writing property values in bulk, checks which element/property pairs can be
# modified and are visible, then the same properties in the Wall and Door tool defaults
# and in the default values of the property definitions themselves.

elements = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements'][:3]

properties = aclib.RunCommand ('API.GetPropertyIds', {'properties': [
    {'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'},
    {'type': 'BuiltIn', 'nonLocalizedName': 'General_Width'}]})['properties']
properties = [p for p in properties if 'propertyId' in p]

result = aclib.RunTapirCommand ('GetPropertyEditability', {
    'properties': properties,
    'elements': elements,
    'elementTypes': ['Wall', 'Door'],
    'propertyDefinitionDefaults': True})
