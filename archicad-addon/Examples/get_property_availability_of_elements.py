import aclib

walls = aclib.RunCommand (
    'API.GetElementsByType', {
        'elementType': 'Wall'
    })['elements']

elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {'properties': [{"type": "BuiltIn", "nonLocalizedName": "General_ElementID"}]})['properties'][0]['propertyId']

allProperties = aclib.RunTapirCommand ('GetAllProperties', {})['properties']
customPropertyIds = [property['propertyId'] for property in allProperties if property['propertyType'] == 'Custom'][:3]

response = aclib.RunTapirCommand (
    'GetPropertyValuesOfElements', {
        'elements': walls,
        'properties': [{
            'propertyId': propertyId
        } for propertyId in [elementIdPropertyId] + customPropertyIds],
        'includeAvailability': True
    })
