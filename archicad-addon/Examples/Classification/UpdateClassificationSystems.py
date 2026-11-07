"""Creates a temporary classification system and changes its version, date and description with UpdateClassificationSystems.

The system keeps its guid. The script deletes it again at the end.
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

aclib.PrintJson (aclib.RunTapirCommand ('UpdateClassificationSystems', {
    'classificationSystems': [{
        'classificationSystemId': systemId['classificationSystemId'],
        'version': '2.0',
        'date': '2026-06-30',
        'description': 'Updated by a Tapir example'
    }]
}))

aclib.RunTapirCommand ('DeleteClassificationSystems', {'classificationSystemIds': [systemId]})
