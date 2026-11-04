"""Creates a View Map folder with two subfolders and swaps their order with ReorderNavigatorItems.

Deletes the folders at the end.
"""
import aclib

parent = aclib.RunTapirCommand ('CreateViewMapFolder', {'folderName': 'Tapir Example Folder'})
first, second = [
    aclib.RunTapirCommand ('CreateViewMapFolder', {'folderName': name, 'parentNavigatorItemId': parent['navigatorItemId']})
    for name in ('Tapir A', 'Tapir B')
]

aclib.PrintJson (aclib.RunTapirCommand ('ReorderNavigatorItems', {
    'parentNavigatorItemId': parent['navigatorItemId'],
    'orderedNavigatorItemIds': [
        {'navigatorItemId': second['navigatorItemId']},
        {'navigatorItemId': first['navigatorItemId']}
    ]
}))

# A folder that is not empty cannot be deleted: delete the subfolders first.
aclib.RunTapirCommand ('DeleteNavigatorItems', {
    'navigatorItemIds': [{'navigatorItemId': first['navigatorItemId']}, {'navigatorItemId': second['navigatorItemId']}]
})
aclib.RunTapirCommand ('DeleteNavigatorItems', {'navigatorItemIds': [{'navigatorItemId': parent['navigatorItemId']}]})
