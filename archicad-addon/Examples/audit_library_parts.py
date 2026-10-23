import aclib

libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Object'}, debug = False)['libraryParts'][:3]

# A library part is given by its index or by the path of its file.
response = aclib.RunTapirCommand ('GetLibraryPartIdentities', {
    'libraryParts': [{'index': libraryPart['index']} for libraryPart in libraryParts]
}, debug = False)
print (response)

unIds = [identity['unId'] for identity in response['identities'] if 'unId' in identity]
if len (unIds) >= 2:
    for unId1, unId2 in [(unIds[0], unIds[0]), (unIds[0], unIds[1])]:
        response = aclib.RunTapirCommand ('CompareLibraryPartUnIds', {
            'unId1': unId1,
            'unId2': unId2
        }, debug = False)
        print (response)
