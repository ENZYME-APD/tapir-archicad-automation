"""
Example for issue #944: the property values of a tool's defaults (the settings new
elements get) can be read and set directly with GetPropertyValuesOfElementDefaults and
SetPropertyValuesOfElementDefaults, without saving a favorite and applying it.

The script reads the Wall tool's default Element ID, sets a new one, creates a wall and
reads the property of the new wall back, then deletes the wall and restores the default.
"""
import aclib

def run (command, parameters = None):
    return aclib.RunTapirCommand (command, parameters or {}, debug = False)

def firstValue (propertyValuesOrError):
    if 'error' in propertyValuesOrError:
        return 'error: ' + propertyValuesOrError['error']['message']
    propertyValue = propertyValuesOrError['propertyValues'][0]
    if 'error' in propertyValue:
        return 'error: ' + propertyValue['error']['message']
    return propertyValue['propertyValue']['value']

elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {'properties': [
    {'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'}]})['properties'][0]['propertyId']

# --- read the current default ------------------------------------------------------
response = run ('GetPropertyValuesOfElementDefaults', {
    'elementTypes': ['Wall'],
    'properties': [{'propertyId': elementIdPropertyId}]
})
previousDefault = firstValue (response['propertyValuesForElementDefaults'][0])
print ('Default Element ID of the Wall tool: {}'.format (previousDefault))

# --- set a new default -------------------------------------------------------------
newDefault = 'WallFromTapirDefaults'
result = run ('SetPropertyValuesOfElementDefaults', {'elementDefaultPropertyValues': [{
    'elementType': 'Wall',
    'propertyId': elementIdPropertyId,
    'propertyValue': {'value': newDefault}
}]})['executionResults'][0]
print ('Setting the default to {}: {}'.format (newDefault, 'succeeded' if result['success'] else result['error']['message']))

# --- a new wall gets the default -----------------------------------------------------
wall = run ('CreateWalls', {'wallsData': [{
    'begCoordinate': {'x': 6000.0, 'y': 0.0},
    'endCoordinate': {'x': 6005.0, 'y': 0.0}
}]})['elements'][0]
if 'elementId' in wall:
    response = run ('GetPropertyValuesOfElements', {
        'elements': [{'elementId': wall['elementId']}],
        'properties': [{'propertyId': elementIdPropertyId}]
    })
    wallValue = firstValue (response['propertyValuesForElements'][0])
    print ('Element ID of the new wall: {} ({})'.format (
        wallValue, 'matches the default' if wallValue == newDefault else 'does not match the default'))
    run ('DeleteElements', {'elements': [{'elementId': wall['elementId']}]})
else:
    print ('Creating the wall failed: {}'.format (wall['error']['message']))

# --- an unknown element type is reported --------------------------------------------
response = run ('GetPropertyValuesOfElementDefaults', {
    'elementTypes': ['NoSuchType'],
    'properties': [{'propertyId': elementIdPropertyId}]
})
print ('Unknown element type: {}'.format (firstValue (response['propertyValuesForElementDefaults'][0])))

# --- restore the previous default ------------------------------------------------------
if not previousDefault.startswith ('error: '):
    result = run ('SetPropertyValuesOfElementDefaults', {'elementDefaultPropertyValues': [{
        'elementType': 'Wall',
        'propertyId': elementIdPropertyId,
        'propertyValue': {'value': previousDefault}
    }]})['executionResults'][0]
    print ('Restoring the default to {}: {}'.format (previousDefault, 'succeeded' if result['success'] else result['error']['message']))
