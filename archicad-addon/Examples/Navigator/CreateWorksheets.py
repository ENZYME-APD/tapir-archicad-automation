"""Creates an independent Worksheet with CreateWorksheets, then deletes it again."""
import aclib

databases = aclib.RunTapirCommand ('CreateWorksheets', {
    'worksheetsData': [{'name': 'Tapir Example Worksheet', 'referenceId': 'TAPIR-W1'}]
})['databases']
aclib.PrintJson (databases)

# Clean up: the new Project Map item is deleted with DeleteNavigatorItems.
aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{'navigatorItemId': d['navigatorItemId']} for d in databases if 'navigatorItemId' in d]
})
