"""Creates a classification item in a temporary classification system and deletes it with DeleteClassificationItems.

The script deletes the temporary system at the end.
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
itemIds = aclib.RunTapirCommand ('CreateClassificationItems', {
    'newClassificationItems': [{
        'classificationSystemId': systemId['classificationSystemId'],
        'classificationItemDetails': {'id': '10', 'name': 'Walls', 'description': ''}
    }]
})['classificationItemIds']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteClassificationItems', {
    'classificationItemIds': [i for i in itemIds if 'classificationItemId' in i]
}))

aclib.RunTapirCommand ('DeleteClassificationSystems', {'classificationSystemIds': [systemId]})
