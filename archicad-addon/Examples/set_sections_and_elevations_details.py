import aclib

# Changes the line pen and the marker text pen of every section of the project.
# Note: Archicad ignores 'name' and 'referenceId' on an existing element; set them at creation time.
sections = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'CutPlane'})['elements']

print (aclib.RunTapirCommand ('SetSectionsAndElevationsDetails', {
    'elementsWithDetails': [{
        'elementId': section['elementId'],
        'details': {'linePenIndex': 5, 'textPenIndex': 5}
    } for section in sections]
}))
