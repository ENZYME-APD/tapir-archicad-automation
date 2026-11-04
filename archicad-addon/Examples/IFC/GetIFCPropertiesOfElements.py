"""Gets the IFC properties of the first wall with GetIFCPropertiesOfElements and prints them by property set.

Needs at least one Wall.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
elementId = walls[0]['elementId']

response = aclib.RunTapirCommand ('GetIFCPropertiesOfElements', {'elements': [{'elementId': elementId}]}, debug = False)
for ifcProperty in response['elementIFCProperties'][0].get ('ifcProperties', []):
    print (ifcProperty['propertySetName'], ifcProperty['name'], '=', ifcProperty['value'])
