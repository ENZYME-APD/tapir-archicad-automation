import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', { 'elementType': 'Wall' })['elements']
zones = aclib.RunTapirCommand ('GetElementsByType', { 'elementType': 'Zone' })['elements']

# All relations of walls and zones
response = aclib.RunTapirCommand ('GetRelationsOfElements', {
    'elements': walls + zones
})

# Relations of zones filtered to walls only
response = aclib.RunTapirCommand ('GetRelationsOfElements', {
    'elements': zones,
    'otherElementType': 'Wall'
})
