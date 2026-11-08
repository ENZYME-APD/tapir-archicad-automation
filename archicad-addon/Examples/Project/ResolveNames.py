"""Checks words from a user's question against the open project with ResolveNames before using them:
a story, a building material, an element type, a property, a classification and a word looked up
among every kind of name. Each result says whether the project has an item named exactly so (found),
only similar names (closeMatches, best first) or nothing close (notFound). A matched property carries
the propertyId that GetPropertyValuesOfElements takes, a matched classification item its
classificationSystemId and classificationItemId."""
import aclib

response = aclib.RunTapirCommand ('ResolveNames', {
    'names': [
        {'name': 'ground floor', 'kind': 'Story'},
        {'name': 'concrete', 'kind': 'BuildingMaterial'},
        {'name': 'door', 'kind': 'ElementType'},
        {'name': 'fire rating', 'kind': 'Property'},
        {'name': 'wall', 'kind': 'Classification'},
        {'name': 'Fire Rating'}
    ],
    'maxCloseMatches': 3
})
aclib.PrintJson (response)

for result in response['results']:
    matches = result['found'] or result['closeMatches']
    print (result['name'], '->', result['status'], [(match['kind'], match['name']) for match in matches])
