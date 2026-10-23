import aclib

# Search for properties matching 'Fire'
response = aclib.RunTapirCommand ('SearchProperties', {
    'query': 'Fire'
})

print ('Found {} properties matching "Fire":'.format (len (response.get ('properties', []))))
for p in response.get ('properties', [])[:5]:
    print ('  - [{}] {} (type: {}, guid: {})'.format (
        p.get ('propertyGroupName'),
        p.get ('propertyName'),
        p.get ('propertyType'),
        p.get ('propertyId', {}).get ('guid')
    ))
