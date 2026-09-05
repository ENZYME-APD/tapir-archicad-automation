import aclib

# Trims the walls under the roofs: every wall and roof in the project goes into one
# TrimElements call, the roofs do the trimming, then the trims are read back and removed.

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
roofs = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Roof'})['elements']

if len (walls) == 0 or len (roofs) == 0:
    print ('The project needs at least one wall and one roof for this example.')
else:
    aclib.RunTapirCommand ('TrimElements', {'elements': walls + roofs})

    trims = aclib.RunTapirCommand ('GetElementTrims', {'elements': walls})['elementTrims']

    pairs = []
    for wall, item in zip (walls, trims):
        for t in item['trimmedBy']:
            pairs.append ({'elementId': wall['elementId'], 'trimmingElementId': t['elementId']})
    if len (pairs) > 0:
        aclib.RunTapirCommand ('RemoveElementTrims', {'elementPairs': pairs})
