import aclib

# A library part is given by its fileName (with extension) returned by
# GetAvailableLibraryParts, or by an element placed from it.
libraryParts = [{'libraryPartName': 'My Object.gsm'}]

objects = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Object'})['elements']
if objects:
    libraryParts.append ({'elementId': objects[0]['elementId']})

response = aclib.RunTapirCommand ('GetLibraryPartScripts', {
    'libraryParts': libraryParts,
    # Leave out 'scripts' to read all five; give 'password' for protected parts.
    'scripts': ['master', '2d', '3d']
}, debug = False)

for libraryPart in response['libraryParts']:
    if 'error' in libraryPart:
        print (libraryPart['error']['message'])
        continue
    print (libraryPart['libraryPartName'], libraryPart.get ('unID'))
    for key, script in libraryPart['scripts'].items ():
        if script['present']:
            print ('---', key, '---')
            print (script['script'])
        else:
            print ('---', key, '(not present) ---')
