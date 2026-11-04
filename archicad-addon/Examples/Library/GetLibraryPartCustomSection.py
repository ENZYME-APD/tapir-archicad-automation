"""Reads the custom section 'TAPR' of the first loaded door library part with GetLibraryPartCustomSection.

Most library parts have no such section, so the response usually says it does not exist. Requires Archicad 28 or later.
"""
import base64

import aclib

doors = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'})['libraryParts']
if not doors:
    raise SystemExit ('No door library part is loaded.')

# A four-character section code is given as its integer value, the first character in the highest byte.
response = aclib.RunTapirCommand ('GetLibraryPartCustomSection', {
    'name': doors[0]['fileName'],
    'sectionId': int.from_bytes (b'TAPR', 'big'),
    'index': 0
})
if response.get ('exists'):
    print (base64.b64decode (response['data']))
