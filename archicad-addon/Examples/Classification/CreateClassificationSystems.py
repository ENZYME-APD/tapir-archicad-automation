"""Creates a classification system with a two-level item tree with CreateClassificationSystems, then deletes it."""
import aclib

result = aclib.RunTapirCommand ('CreateClassificationSystems', {
    'classificationSystemsWithItems': [{
        'classificationSystem': {
            'name': 'Tapir Example System',
            'description': 'Created by a Tapir example',
            'source': 'https://github.com/ENZYME-APD/tapir-archicad-automation',
            'version': '1.0',
            'date': '2026-01-01'
        },
        'classificationItems': [{
            'id': '10',
            'name': 'Walls',
            'description': '',
            'children': [
                {'id': '10.1', 'name': 'External walls', 'description': ''},
                {'id': '10.2', 'name': 'Internal walls', 'description': ''}
            ]
        }]
    }]
})
aclib.PrintJson (result)

aclib.RunTapirCommand ('DeleteClassificationSystems', {
    'classificationSystemIds': [s for s in result['classificationSystemIds'] if 'classificationSystemId' in s]
})
