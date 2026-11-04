"""Adds a classification item to a temporary classification system with CreateClassificationItems.

The script deletes the system with its items again at the end.
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

aclib.PrintJson (aclib.RunTapirCommand ('CreateClassificationItems', {
    'newClassificationItems': [{
        'classificationSystemId': systemId['classificationSystemId'],
        'classificationItemDetails': {'id': '20', 'name': 'Slabs', 'description': 'Added by a Tapir example'}
    }]
}))

aclib.RunTapirCommand ('DeleteClassificationSystems', {'classificationSystemIds': [systemId]})
