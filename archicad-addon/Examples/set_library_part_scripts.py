import aclib

# The name is the fileName (with extension) returned by GetAvailableLibraryParts.
# Only library parts loaded from a writable file can be modified, e.g. your own
# parts in a local library folder; parts of a library package are refused.
libraryPartName = 'My Object.gsm'

response = aclib.RunTapirCommand ('SetLibraryPartScripts', {
    'libraryParts': [{
        'name': libraryPartName,
        'scripts': {
            # Only the given scripts are changed; an empty string makes the 2D script empty.
            '3d': 'BLOCK A, B, ZZYZX\n',
            '2d': ''
        }
    }]
}, debug = False)
print (response)
