import aclib

# SetLibraryPartParameters needs Archicad 28 or newer.
# The name is the fileName (with extension) returned by GetAvailableLibraryParts.
# Only library parts loaded from a writable file can be modified, e.g. your own
# parts in a local library folder; parts of a library package are refused.
libraryPartName = 'My Object.gsm'

response = aclib.RunTapirCommand ('SetLibraryPartParameters', {
    'libraryParts': [{
        'name': libraryPartName,
        # The format of SetGDLParametersOfElements: only the given parameters are changed.
        'parameters': [
            { 'name': 'A', 'value': 1.2 },
            { 'name': 'B', 'value': 0.6 }
        ]
    }]
}, debug = False)
print (response)
