"""Gives the first wall without a web link a link with SetURLReferencesOfElements and reads it back.

Needs a Wall without a link. The link stays on the wall: this command cannot replace or remove an
existing link.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
if not walls:
    raise SystemExit ('No wall in the project.')

links = aclib.RunTapirCommand ('GetURLReferencesOfElements', {'elements': walls})['urlReferences']
wallsWithoutLink = [wall for wall, link in zip (walls, links) if 'error' in link]
if not wallsWithoutLink:
    raise SystemExit ('Every wall has a link already.')

aclib.PrintJson (aclib.RunTapirCommand ('SetURLReferencesOfElements', {'elementsWithURLReferences': [{
    'elementId': wallsWithoutLink[0]['elementId'],
    'url': 'https://www.graphisoft.com',
    'description': 'GRAPHISOFT Home Page'
}]}))
aclib.RunTapirCommand ('GetURLReferencesOfElements', {'elements': wallsWithoutLink[:1]})
