"""Creates a keynote folder and deletes it with DeleteKeynoteFolders.

Deleting a folder deletes its content too. Requires Archicad 28 or later.
"""
import aclib

folder = aclib.RunTapirCommand ('CreateKeynoteFolders', {
    'foldersData': [{'key': 'TAPIR', 'title': 'Tapir Example Folder'}]
})['keynoteFolderIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('DeleteKeynoteFolders', {'keynoteFolderIds': [{'keynoteFolderId': folder['keynoteFolderId']}]}))
