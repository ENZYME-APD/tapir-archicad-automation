import aclib

# Elements can carry a web link (a URL with a description), shown in Archicad in the
# element's settings under Element Information > Link. GetURLReferencesOfElements reads it,
# SetURLReferencesOfElements sets it on elements that have none yet. Typical uses are links
# to product data sheets, BIM object pages or issue trackers.

elements = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']

if len (elements) == 0:
    print ('No walls in the project.')
else:
    urlReferences = aclib.RunTapirCommand (
        'GetURLReferencesOfElements', {'elements': elements})['urlReferences']

    # Elements without a link get an error entry (APIERR_NOUSERDATA); only those get a link here.
    # Setting a link on an element that already has one fails with APIERR_MOREUSER.
    elementsWithURLReferences = [{
        'elementId': element['elementId'],
        'url': 'https://www.graphisoft.com',
        'description': 'GRAPHISOFT Home Page'
    } for element, urlReference in zip (elements, urlReferences) if 'error' in urlReference]

    if len (elementsWithURLReferences) > 0:
        aclib.RunTapirCommand (
            'SetURLReferencesOfElements', {'elementsWithURLReferences': elementsWithURLReferences})

        aclib.RunTapirCommand ('GetURLReferencesOfElements', {'elements': elements})
