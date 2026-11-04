"""Creates a classification item in a temporary system and changes its code and name with UpdateClassificationItems.

The item keeps its guid, so the elements classified with it stay classified. The script deletes the temporary
system at the end.
"""
import aclib

systemId = aclib.RunTapirCommand ('CreateClassificationSystems', {
    'classificationSystemsWithItems': [{
        'classificationSystem': {'name': 'Tapir Example System', 'description': '', 'source': '', 'version': '1.0', 'date': '2026-01-01'},
        'classificationItems': []
    }]
})['classificationSystemIds'][0]
if 'error' in systemId:
    raise SystemExit ('Could not create the classification system: ' + systemId['error']['message'])
itemId = aclib.RunTapirCommand ('CreateClassificationItems', {
    'newClassificationItems': [{
        'classificationSystemId': systemId['classificationSystemId'],
        'classificationItemDetails': {'id': '10', 'name': 'Walls', 'description': ''}
    }]
})['classificationItemIds'][0]

aclib.PrintJson (aclib.RunTapirCommand ('UpdateClassificationItems', {
    'classificationItems': [{
        'classificationItemId': itemId['classificationItemId'],
        'id': '21.10',
        'name': 'Load-bearing walls'
    }]
}))

aclib.RunTapirCommand ('DeleteClassificationSystems', {'classificationSystemIds': [systemId]})
