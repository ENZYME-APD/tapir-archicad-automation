"""Creates a keynote folder with a keynote item in it, using CreateKeynoteItems.

Deletes the folder and the item at the end. Requires Archicad 28 or later.
"""
import aclib

folder = aclib.RunTapirCommand ('CreateKeynoteFolders', {
    'foldersData': [{'key': 'TAPIR', 'title': 'Tapir Example Folder'}]
})['keynoteFolderIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateKeynoteItems', {
    'itemsData': [{
        'parentFolderId': folder['keynoteFolderId'],
        'key': 'TAPIR-01',
        'title': 'Tapir Example Keynote',
        'description': 'Created by the Tapir example.',
        'reference': 'EX-1'
    }]
}))

aclib.RunTapirCommand ('DeleteKeynoteFolders', {'keynoteFolderIds': [{'keynoteFolderId': folder['keynoteFolderId']}]})
