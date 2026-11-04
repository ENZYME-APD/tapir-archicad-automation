"""Creates a folder at the root of the View Map with CreateViewMapFolder, then deletes it again."""
import aclib

folder = aclib.RunTapirCommand ('CreateViewMapFolder', {'folderName': 'Tapir Example Folder'})
aclib.PrintJson (folder)

aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': folder['navigatorItemId']}]})
