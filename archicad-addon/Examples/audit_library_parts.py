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

# The sections of the first library part, and the text of its first text section.
if libraryParts:
    response = aclib.RunTapirCommand ('GetLibraryPartSections', {
        'libraryParts': [{'index': libraryParts[0]['index']}]
    }, debug = False)
    print (response)

    sections = response['libraryParts'][0].get ('sections', [])
    textSections = [section for section in sections if section.get ('isText') and section.get ('size', 0) > 0]
    if textSections:
        response = aclib.RunTapirCommand ('GetLibraryPartSection', {
            'index': libraryParts[0]['index'],
            'sectionType': textSections[0]['sectionType'],
            'subIdent': textSections[0]['subIdent']
        }, debug = False)
        print (response)
