"""Prints the settings of the first 3D Document of the Project Map with Get3DDocumentSettings.

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

aclib.PrintJson (aclib.RunTapirCommand ('Get3DDocumentSettings', {'navigatorItemId': documents[0]['navigatorItemId']}))
