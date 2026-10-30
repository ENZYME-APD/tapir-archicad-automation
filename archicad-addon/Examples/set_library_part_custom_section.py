import aclib
import base64

# The name is the fileName (with extension) returned by GetAvailableLibraryParts.
# Only library parts loaded from a writable file can be modified, e.g. your own
# parts in a local library folder; parts of a library package are refused.
libraryPartName = 'My Object.gsm'

# A four-character section code is given as its integer value, the first character in the highest byte.
sectionId = int.from_bytes (b'TAPR', 'big')

# Creates the section, or replaces it when it exists.
response = aclib.RunTapirCommand ('SetLibraryPartCustomSection', {
    'name': libraryPartName,
    'sectionId': sectionId,
    'index': 0,
    'dataVersion': 1,
    'data': base64.b64encode (b'Data stored by Tapir').decode ('ascii')
}, debug = False)
print (response)

# Deletes it again.
response = aclib.RunTapirCommand ('SetLibraryPartCustomSection', {
    'name': libraryPartName,
    'sectionId': sectionId,
    'delete': True
}, debug = False)
print (response)
