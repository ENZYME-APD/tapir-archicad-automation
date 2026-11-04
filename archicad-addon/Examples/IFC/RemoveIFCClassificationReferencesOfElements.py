"""Removes an IFC classification reference from the first wall with RemoveIFCClassificationReferencesOfElements.

Needs at least one Wall. The removed reference is stored by the script first. Available only in Archicad 25, 26 and 27.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
elementId = walls[0]['elementId']

aclib.RunTapirCommand ('SetIFCClassificationReferencesOfElements', {
    'elementIFCClassificationReferences': [{
        'elementId': elementId,
        'identification': 'TE-01',
        'name': 'Tapir Example Item',
        'location': 'https://example.com/classification/TE-01',
        'referencedSource': {
            'name': 'Tapir Example Classification',
            'edition': '1.0',
            'editionDate': '2026-01-01',
            'description': 'A classification made up for this example.',
            'location': 'https://example.com/classification'
        }
    }]
})

aclib.PrintJson (aclib.RunTapirCommand ('RemoveIFCClassificationReferencesOfElements', {
    'elementIFCClassificationReferences': [{
        'elementId': elementId,
        'referencedSource': {'name': 'Tapir Example Classification'},
        'identification': 'TE-01'
    }]
}))
