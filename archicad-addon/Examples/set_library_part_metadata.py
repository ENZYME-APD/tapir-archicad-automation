import aclib

# The name is the fileName (with extension) returned by GetAvailableLibraryParts.
# Only library parts loaded from a writable file can be modified, e.g. your own
# parts in a local library folder; parts of a library package are refused.
libraryPartName = 'My Object.gsm'

response = aclib.RunTapirCommand ('SetLibraryPartMetadata', {
    'libraryParts': [{
        'name': libraryPartName,
        'metadata': {
            # Only the given fields are changed.
            'author': 'Tapir',
            'keywords': 'tapir example',
            'placeable': True
        }
    }]
}, debug = False)
print (response)
