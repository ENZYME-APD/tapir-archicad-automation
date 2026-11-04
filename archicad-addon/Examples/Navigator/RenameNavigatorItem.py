"""Creates a View Map folder and renames it with RenameNavigatorItem.

Deletes the folder at the end.
"""
import aclib

folder = aclib.RunTapirCommand ('CreateViewMapFolder', {'folderName': 'Tapir Example Folder'})

aclib.PrintJson (aclib.RunTapirCommand ('RenameNavigatorItem', {
    'navigatorItemId': folder['navigatorItemId'],
    'newName': 'Tapir Renamed Folder'
}))

aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': folder['navigatorItemId']}]})
