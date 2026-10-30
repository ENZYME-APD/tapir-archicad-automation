import aclib

# Only the custom properties, without their availability lists.
response = aclib.RunTapirCommand ('GetAllProperties', {
    'propertyTypes': ['Custom'],
    'includeAvailability': False
}, debug = False)
custom = response['properties']
print ('Only custom properties: {}'.format (all (p['propertyType'] == 'Custom' for p in custom)))
print ('No availability lists: {}'.format (all ('availability' not in p for p in custom)))

# The definitions of the given properties only, e.g. to read their expressions.
if custom:
    wanted = custom[:2]
    response = aclib.RunTapirCommand ('GetAllProperties', {
        'properties': [{'propertyId': p['propertyId']} for p in wanted]
    }, debug = False)
    returnedGuids = sorted (p['propertyId']['guid'] for p in response['properties'])
    print ('Returned exactly the requested properties: {}'.format (
        returnedGuids == sorted (p['propertyId']['guid'] for p in wanted)))
    for p in response['properties']:
        if p['isExpressionBased']:
            print ('{}/{} expressions: {}'.format (p['propertyGroupName'], p['propertyName'], p['expressions']))
