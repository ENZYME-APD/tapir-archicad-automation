import aclib

response = aclib.RunTapirCommand ('GetAllProperties', {})

custom = [p for p in response['properties'] if p['propertyType'] == 'Custom']
if custom:
    p = custom[0]
    print ('First custom property: {}/{}'.format (p['propertyGroupName'], p['propertyName']))
    print ('  has group id: {}'.format ('propertyGroupId' in p))
    print ('  availability entries: {}'.format (len (p.get ('availability', []))))
print ('Property groups listed: {}'.format (len (response.get ('propertyGroups', [])) > 0))
