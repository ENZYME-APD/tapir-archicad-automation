import aclib

# The names are the fileNames (with extension) returned by GetAvailableLibraryParts.
# Only library parts loaded from a writable file can be modified, e.g. your own
# parts in a local library folder; parts of a library package are refused.
libraryPartName = 'My Object.gsm'
symbolSourceName = 'My Other Object.gsm'

response = aclib.RunTapirCommand ('SetLibraryPartPictures', {
    'name': libraryPartName,
    'deleteGdlPictures': True,
    # Replaces the 2D symbol of the part with the 2D symbol of another library part.
    'copy2DSymbolFrom': symbolSourceName
}, debug = False)
print (response)
