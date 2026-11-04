import aclib

# Requires Archicad 28 or newer.

# Pick a pipe segment preference table and two of its rows. The table guid and the
# referenceId of a row are what CreateMEPRoutingElements / ModifyMEPRoutingElements
# accept as preferenceTableId / crossSectionReferenceId.
pipeTables = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
        'domain': 'Piping'
    })['tables']

table = next ((t for t in pipeTables if len (t['rows']) >= 2), None)
if table is None:
    print ('No pipe preference table with at least two rows found.')
    raise SystemExit

firstRow = table['rows'][0]
secondRow = table['rows'][1]
print (f"Using pipe preference table {table['guid']}")

# Create a pipe route that uses the chosen table and the cross section of its first row
createdRoutes = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    { 'x': 0.0, 'y': -4.0, 'z': 1.0 },
                    { 'x': 6.0, 'y': -4.0, 'z': 1.0 }
                ],
                'preferenceTableId': table['guid'],
                'crossSectionReferenceId': firstRow['referenceId']
            }
        ]
    })['elements']

routeIds = [e['elementId'] for e in createdRoutes if 'elementId' in e]
if not routeIds:
    print ('Failed to create the pipe route: {}'.format (createdRoutes))
    raise SystemExit

def PrintSegments (title):
    details = aclib.RunTapirCommand ('GetMEPRoutingElements', {
            'elements': [{ 'elementId': routeIds[0] }]
        })['routingElements'][0]
    print (title)
    for segment in details['segments']:
        print ('  table={} referenceId={} width={}'.format (
            segment.get ('preferenceTableId'), segment.get ('crossSectionReferenceId'), segment['crossSectionWidth']))

PrintSegments ('After create (expected referenceId {}, diameter {}):'.format (firstRow['referenceId'], firstRow['diameter']))

# Switch every segment of the route to the second row of the same table, without
# recreating the route
aclib.RunTapirCommand ('ModifyMEPRoutingElements', {
        'routingElementsData': [
            {
                'elementId': routeIds[0],
                'preferenceTableId': table['guid'],
                'crossSectionReferenceId': secondRow['referenceId']
            }
        ]
    })

PrintSegments ('After modify (expected referenceId {}, diameter {}):'.format (secondRow['referenceId'], secondRow['diameter']))

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{ 'elementId': routeId } for routeId in routeIds]
    })
