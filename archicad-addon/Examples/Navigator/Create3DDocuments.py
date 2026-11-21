"""Creates a 3D Document with Create3DDocuments, prints its settings with Get3DDocumentSettings, then deletes it again."""
import aclib

databases = aclib.RunTapirCommand ('Create3DDocuments', {
    'documentsData': [{'name': 'Tapir Example 3D Document', 'referenceId': 'TAPIR-3D1'}]
})['databases']
aclib.PrintJson (databases)

for database in databases:
    if 'navigatorItemId' in database:
        aclib.PrintJson (aclib.RunTapirCommand ('Get3DDocumentSettings', {'navigatorItemId': database['navigatorItemId']}))

# Clean up: the new Project Map item is deleted with DeleteNavigatorItems.
aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{'navigatorItemId': d['navigatorItemId']} for d in databases if 'navigatorItemId' in d]
})
