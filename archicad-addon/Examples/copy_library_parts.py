import aclib

# A library part is given by its fileName (with extension) returned by
# GetAvailableLibraryParts, or by an element placed from it. Without
# 'targetFolder' the copy goes into the embedded library.
libraryParts = [{'libraryPartName': 'My Object.gsm', 'newName': 'My Object copy.gsm'}]

objects = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Object'})['elements']
if objects:
    libraryParts.append ({'elementId': objects[0]['elementId'], 'newName': 'Placed Object copy.gsm', 'keepMainGuid': False})

response = aclib.RunTapirCommand ('CopyLibraryParts', {
    'libraryParts': libraryParts
}, debug = False)

for libraryPart in response['libraryParts']:
    if 'error' in libraryPart:
        print (libraryPart['error']['message'])
        continue
    print (libraryPart['libraryPartName'], libraryPart.get ('unID'), libraryPart['path'])

print ('Libraries reloaded:', response['librariesReloaded'])
