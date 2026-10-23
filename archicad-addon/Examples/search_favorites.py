import aclib

# Search for favorites matching 'Wall'
response = aclib.RunTapirCommand ('SearchFavorites', {
    'query': 'Wall'
})

print ('Found {} favorites matching "Wall":'.format (len (response.get ('favorites', []))))
for f in response.get ('favorites', [])[:5]:
    folder_path = ' / '.join (f.get ('folder', []))
    print ('  - {} (type: {}, path: {})'.format (
        f.get ('name'),
        f.get ('elementType'),
        folder_path
    ))
