import aclib

# Separating the project's own elements from the ones that came in through
# placed hotlinks (fromHotlink), and picking the elements of one placed
# hotlink instance (hotlinkIds). The results are compared to the hotlinkId
# field of GetDetailsOfElements, which they must match.

allElements = aclib.RunTapirCommand ('GetAllElements', {}, debug=False)['elements']
details = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': allElements,
    'fields': ['hotlinkId']
}, debug=False)['detailsOfElements']
hotlinkIdByGuid = {
    element['elementId']['guid'].lower (): detail.get ('hotlinkId', {}).get ('guid', '').lower ()
    for element, detail in zip (allElements, details)
}

def FilteredGuids (criteria):
    response = aclib.RunTapirCommand ('FilterElements', {'elements': allElements, **criteria}, debug=False)
    return sorted (element['elementId']['guid'].lower () for element in response['elements'])

def Check (description, criteria, predicate):
    filtered = FilteredGuids (criteria)
    expected = sorted (guid for guid, hotlinkId in hotlinkIdByGuid.items () if predicate (hotlinkId))
    result = 'matches' if filtered == expected else 'DIFFERS from'
    print ('{}: {} elements, {} GetDetailsOfElements'.format (description, len (filtered), result))

Check ('fromHotlink true', {'fromHotlink': True}, lambda hotlinkId: hotlinkId != '')
Check ('fromHotlink false', {'fromHotlink': False}, lambda hotlinkId: hotlinkId == '')

# The placed instances of each hotlink node are listed by GetHotlinks.
def CollectInstances (hotlinks):
    for hotlink in hotlinks:
        yield from hotlink.get ('instances', [])
        yield from CollectInstances (hotlink.get ('children', []))

instances = list (CollectInstances (aclib.RunTapirCommand ('GetHotlinks', debug=False)['hotlinks']))
print ('Placed hotlink instances: {}'.format (len (instances)))
for instance in instances[:3]:
    instanceGuid = instance['elementId']['guid'].lower ()
    Check ('hotlinkIds [{}]'.format (instanceGuid), {'hotlinkIds': [instance['elementId']]},
           lambda hotlinkId: hotlinkId == instanceGuid)
