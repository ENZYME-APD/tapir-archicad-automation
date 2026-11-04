"""Creates a temporary classification system and deletes it again with DeleteClassificationSystems."""
import aclib

result = aclib.RunTapirCommand ('CreateClassificationSystems', {
    'classificationSystemsWithItems': [{
        'classificationSystem': {'name': 'Tapir Example System To Delete', 'description': '', 'source': '', 'version': '1.0', 'date': '2026-01-01'},
        'classificationItems': [{'id': '10', 'name': 'Walls', 'description': ''}]
    }]
})

aclib.PrintJson (aclib.RunTapirCommand ('DeleteClassificationSystems', {
    'classificationSystemIds': [s for s in result['classificationSystemIds'] if 'classificationSystemId' in s]
}))
