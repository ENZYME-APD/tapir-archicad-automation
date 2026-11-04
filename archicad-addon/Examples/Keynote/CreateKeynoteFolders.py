"""Creates a keynote folder with a subfolder in it, using CreateKeynoteFolders.

Deletes the folders at the end. Requires Archicad 28 or later.
"""
import aclib

folder = aclib.RunTapirCommand ('CreateKeynoteFolders', {
    'foldersData': [{'key': 'TAPIR', 'title': 'Tapir Example Folder'}]
})['keynoteFolderIdsOrErrors'][0]
aclib.PrintJson (folder)

# Without parentFolderId the folder is created in the root folder.
aclib.PrintJson (aclib.RunTapirCommand ('CreateKeynoteFolders', {
    'foldersData': [{'parentFolderId': folder['keynoteFolderId'], 'key': 'TAPIR-SUB', 'title': 'Tapir Example Subfolder'}]
}))

# Deleting a folder deletes its content too.
aclib.RunTapirCommand ('DeleteKeynoteFolders', {'keynoteFolderIds': [{'keynoteFolderId': folder['keynoteFolderId']}]})
