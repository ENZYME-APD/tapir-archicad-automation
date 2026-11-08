"""Creates two View Map folders and moves the second into the first with MoveNavigatorItem.

Deletes both folders at the end.
"""
import aclib

first = aclib.RunTapirCommand ('CreateViewMapFolder', {'folderName': 'Tapir Example Folder A'})
second = aclib.RunTapirCommand ('CreateViewMapFolder', {'folderName': 'Tapir Example Folder B'})

aclib.PrintJson (aclib.RunTapirCommand ('MoveNavigatorItem', {
    'navigatorItemIdToMove': second['navigatorItemId'],
    'parentNavigatorItemId': first['navigatorItemId']
}))
aclib.RunTapirCommand ('GetNavigatorParentItems', {'navigatorItemIds': [{'navigatorItemId': second['navigatorItemId']}]})

# A folder that is not empty cannot be deleted: delete the inner one first.
aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': second['navigatorItemId']}]})
aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': first['navigatorItemId']}]})
