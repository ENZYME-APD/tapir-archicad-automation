"""Changes one marker parameter of a window or door with SetMarkerParametersOfElements, then restores it.

Needs at least one Window or Door whose marker has a String parameter.
"""
import aclib

openings = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Window'})['elements']
openings += aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Door'})['elements']
if not openings:
    raise SystemExit ('No window or door in the project.')

opening = openings[0]
marker = aclib.RunTapirCommand ('GetMarkerParametersOfElements', {'elements': [opening]})['markerParametersOfElements'][0]
stringParameters = [p for p in marker.get ('parameters', []) if p['type'] == 'String' and 'value' in p and 'dimension1' not in p]
if not stringParameters:
    raise SystemExit ('The marker of the first opening has no String parameter.')

parameter = stringParameters[0]
aclib.PrintJson (aclib.RunTapirCommand ('SetMarkerParametersOfElements', {'elementsWithMarkerParameters': [{
    'elementId': opening['elementId'],
    'markerParameters': [{'name': parameter['name'], 'value': 'Tapir'}]
}]}))
aclib.RunTapirCommand ('GetMarkerParametersOfElements', {'elements': [opening]})

aclib.PrintJson (aclib.RunTapirCommand ('SetMarkerParametersOfElements', {'elementsWithMarkerParameters': [{
    'elementId': opening['elementId'],
    'markerParameters': [{'name': parameter['name'], 'value': parameter['value']}]
}]}))
