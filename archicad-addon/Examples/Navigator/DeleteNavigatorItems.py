"""Creates an independent Worksheet and deletes it from the Project Map with DeleteNavigatorItems."""
import aclib

worksheet = aclib.RunTapirCommand ('CreateWorksheets', {
    'worksheetsData': [{'name': 'Tapir Example Worksheet', 'referenceId': 'TAPIR-W1'}]
})['databases'][0]

aclib.PrintJson (aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{'navigatorItemId': worksheet['navigatorItemId']}]
}))
