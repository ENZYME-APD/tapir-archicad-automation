"""Compares the unique IDs of the first two door library parts with CompareLibraryPartUnIds.

Needs at least two loaded door library parts.
"""
import aclib

doors = aclib.RunTapirCommand ('GetAvailableLibraryParts', {'filterByTypeId': 'Door'})['libraryParts']
identities = aclib.RunTapirCommand ('GetLibraryPartIdentities', {
    'libraryParts': [{'index': door['index']} for door in doors[:2]]
})['identities']
unIds = [identity['unId'] for identity in identities if 'unId' in identity]
if len (unIds) < 2:
    raise SystemExit ('Fewer than two door library parts are loaded.')

aclib.PrintJson (aclib.RunTapirCommand ('CompareLibraryPartUnIds', {'unId1': unIds[0], 'unId2': unIds[1]}))
