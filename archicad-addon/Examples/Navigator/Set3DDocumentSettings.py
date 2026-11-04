"""Switches the transparency of the first 3D Document with Set3DDocumentSettings, then restores it.

Needs at least one 3D Document.
"""
import aclib

def AllItems (item):
    for child in item.get ('children', []):
        yield child['navigatorItem']
        yield from AllItems (child['navigatorItem'])

projectMap = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'ProjectMap'})['navigatorItemTree']
documents = [item for item in AllItems (projectMap) if item['type'] == 'DocumentFrom3DItem']
if not documents:
    raise SystemExit ('No 3D Document in the Project Map.')
navigatorItemId = documents[0]['navigatorItemId']

settings = aclib.RunTapirCommand ('Get3DDocumentSettings', {'navigatorItemId': navigatorItemId})['settings']

# Only the given keys change; the response holds the settings read back.
aclib.PrintJson (aclib.RunTapirCommand ('Set3DDocumentSettings', {
    'navigatorItemId': navigatorItemId,
    'settings': {'transparency': not settings['transparency']}
}))

aclib.PrintJson (aclib.RunTapirCommand ('Set3DDocumentSettings', {
    'navigatorItemId': navigatorItemId,
    'settings': {'transparency': settings['transparency']}
}))
