import aclib
import base64

# The name is the fileName (with extension) returned by GetAvailableLibraryParts.
libraryPartName = 'My Object.gsm'

# A four-character section code is given as its integer value, the first character in the highest byte.
sectionId = int.from_bytes (b'TAPR', 'big')

response = aclib.RunTapirCommand ('GetLibraryPartCustomSection', {
    'name': libraryPartName,
    'sectionId': sectionId,
    'index': 0
}, debug = False)
print (response)

if response and response.get ('exists'):
    print (base64.b64decode (response['data']))
