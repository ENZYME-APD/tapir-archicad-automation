import aclib

# GetZoneReductions returns the area reductions of Zones as the Zone Stamp's area calculation
# uses them: the polygons of the Zone covered by Walls, Columns and reducing Fills, and the
# low ceiling parts, for example under a sloping Roof (Gable), each with the percentage it is
# reduced by and its area.
# GetQuantitiesOfElements only gives the reduced totals; the parts are needed for area
# calculations by local standards.

zones = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Zone'})['elements']

response = aclib.RunTapirCommand ('GetZoneReductions', {
    'elements': zones
}, debug = False)

for zone, reductionsOfZone in zip (zones, response['reductionsOfZones']):
    if 'error' in reductionsOfZone:
        print ('Zone {}: error\n{}'.format (zone['elementId']['guid'], aclib.JsonDumpDictionary (reductionsOfZone['error'])))
        continue
    reductions = reductionsOfZone['reductions']
    print ('Zone {} has {} reductions'.format (zone['elementId']['guid'], len (reductions)))
    for reduction in reductions:
        print ('  {}: {:.1f}%, area = {:.3f} m2, outline of {} points with {} holes'.format (
            reduction['type'], reduction['percent'], reduction['area'],
            len (reduction['polygonOutline']), len (reduction['holes'])))
