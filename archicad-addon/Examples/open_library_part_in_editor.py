import aclib

# The name is the fileName (with extension) or the documentName returned by GetAvailableLibraryParts.
libraryPartName = 'My Object.gsm'

response = aclib.RunTapirCommand ('OpenLibraryPartInEditor', {
    'name': libraryPartName
}, debug = False)
print (response)
