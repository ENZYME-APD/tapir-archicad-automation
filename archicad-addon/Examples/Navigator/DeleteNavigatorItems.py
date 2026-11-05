"""Creates an independent Worksheet and deletes it from the Project Map with DeleteNavigatorItems, first as a dry run that deletes nothing."""
import aclib

worksheet = aclib.RunTapirCommand ('CreateWorksheets', {
    'worksheetsData': [{'name': 'Tapir Example Worksheet', 'referenceId': 'TAPIR-W1'}]
})['databases'][0]

parameters = {
    'navigatorItemIds': [{'navigatorItemId': worksheet['navigatorItemId']}]
}
aclib.PrintJson (aclib.RunTapirCommand ('DeleteNavigatorItems', dict (parameters, dryRun = True)))
aclib.PrintJson (aclib.RunTapirCommand ('DeleteNavigatorItems', parameters))
