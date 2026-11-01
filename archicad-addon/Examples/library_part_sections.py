import aclib

# The name of a loaded library part, without the file extension. Only parts loaded from a
# writable file can be modified, e.g. your own parts in a local library folder.
libraryPartName = 'My Object'

response = aclib.RunTapirCommand ('GetLibraryPart2DPrimitives', {
    'name': libraryPartName
}, debug = False)
print (response)

response = aclib.RunTapirCommand ('UpdateLibraryPartSections', {
    'name': libraryPartName,
    'sections': [
        # Only sections the library part already has are replaced.
        { 'type': '3DScript', 'text': 'BLOCK A, B, ZZYZX\n' },
        { 'type': 'Comment', 'text': 'Updated by Tapir' }
    ]
}, debug = False)
print (response)
