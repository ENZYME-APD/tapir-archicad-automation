"""Creates a keynote folder and changes its title with ModifyKeynoteFolders.

Deletes the folder at the end. Requires Archicad 28 or later.
"""
import aclib

folder = aclib.RunTapirCommand ('CreateKeynoteFolders', {
    'foldersData': [{'key': 'TAPIR', 'title': 'Tapir Example Folder'}]
})['keynoteFolderIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyKeynoteFolders', {
    'foldersData': [{'keynoteFolderId': folder['keynoteFolderId'], 'title': 'Tapir Renamed Folder'}]
}))

aclib.RunTapirCommand ('DeleteKeynoteFolders', {'keynoteFolderIds': [{'keynoteFolderId': folder['keynoteFolderId']}]})
