import aclib

# 1. Count elements matching criteria with minimal token usage (countOnly: True)
count_response = aclib.RunTapirCommand ('GetElementsByCriteria', {
    'types': ['Wall'],
    'countOnly': True
})
print ('Matching wall count:', count_response.get ('count', 0))

# 2. Query matching element IDs with story or property criteria
elements_response = aclib.RunTapirCommand ('GetElementsByCriteria', {
    'types': ['Wall'],
    'floorIndices': [0],
    'limit': 10
})
matching_elements = elements_response.get ('elements', [])
print ('Fetched {} matching elements on Ground Floor:'.format (len (matching_elements)))
for elem in matching_elements[:5]:
    print ('  - GUID:', elem.get ('elementId', {}).get ('guid'))
