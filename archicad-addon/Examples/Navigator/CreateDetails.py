"""Creates an independent Detail with CreateDetails, then deletes it again."""
import aclib

databases = aclib.RunTapirCommand ('CreateDetails', {
    'detailsData': [{'name': 'Tapir Example Detail', 'referenceId': 'TAPIR-D1'}]
})['databases']
aclib.PrintJson (databases)

# Clean up: the new Project Map item is deleted with DeleteNavigatorItems.
aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{'navigatorItemId': d['navigatorItemId']} for d in databases if 'navigatorItemId' in d]
})
