"""Creates a straight railing, then gives it a new reference line with two more nodes with RecreateRailings, keeping its settings and Element ID.

The Archicad API cannot change the reference line of an existing railing, so RecreateRailings creates
it again with the old one as template and deletes the old one: the result gives the new railing and
lists what could not be carried over (labels, dimensions, group). coordinates is a 2D reference line as
CreateRailings takes it; polylineCoordinates (with polylineArcs) gives each node its own elevation.
"""
import aclib

railing = aclib.RunTapirCommand ('CreateRailings', {'railingsData': [{
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}],
    'height': 1.1,
    'id': 'RAIL-01'
}]})['elements'][0]

result = aclib.RunTapirCommand ('RecreateRailings', {'railings': [{
    'elementId': railing['elementId'],
    'coordinates': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}, {'x': 4.0, 'y': 3.0}, {'x': 7.0, 'y': 3.0}]
}]})
aclib.PrintJson (result)

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [{'elementId': result['results'][0]['elementId']}]}))
