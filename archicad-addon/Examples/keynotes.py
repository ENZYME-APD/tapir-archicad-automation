import aclib

# Requires Archicad 28 or newer.

# Dump the current keynote tree
def PrintFolder (folder, indent = 0):
    print ('  ' * indent + 'Folder: key="{}" title="{}"'.format (folder['key'], folder['title']))
    for item in folder['items']:
        print ('  ' * (indent + 1) + 'Item: key="{}" title="{}"'.format (item['key'], item['title']))
    for subFolder in folder['subFolders']:
        PrintFolder (subFolder, indent + 1)

rootFolder = aclib.RunTapirCommand ('GetKeynoteTree')['rootFolder']
PrintFolder (rootFolder)

# Create a folder and an item inside it
newFolderId = aclib.RunTapirCommand ('CreateKeynoteFolders', {
        'foldersData': [{ 'key': 'TAPIR', 'title': 'Tapir Example Folder' }]
    })['keynoteFolderIdsOrErrors'][0]['keynoteFolderId']

newItemId = aclib.RunTapirCommand ('CreateKeynoteItems', {
        'itemsData': [{
            'parentFolderId': newFolderId,
            'key': 'TAPIR-01',
            'title': 'Example Keynote',
            'description': 'Created by the Tapir keynotes example.',
            'reference': 'EX-1'
        }]
    })['keynoteItemIdsOrErrors'][0]['keynoteItemId']

# Modify them
aclib.RunTapirCommand ('ModifyKeynoteFolders', {
        'foldersData': [{ 'keynoteFolderId': newFolderId, 'title': 'Tapir Example Folder (renamed)' }]
    })
aclib.RunTapirCommand ('ModifyKeynoteItems', {
        'itemsData': [{ 'keynoteItemId': newItemId, 'title': 'Example Keynote (renamed)' }]
    })

# Get the autotext tokens of the new item and place a label using them
autoTexts = aclib.RunTapirCommand ('GetKeynoteAutoTexts', {
        'keynoteItems': [{ 'keynoteItemId': newItemId }]
    })['autoTexts']
print ('AutoText tokens: {}'.format (autoTexts[0]))

aclib.RunTapirCommand ('CreateKeynoteLabels', {
        'labelsData': [{
            'keynoteItemId': newItemId,
            'position': { 'x': 0.0, 'y': 0.0 },
            'contentFields': ['Key', 'Title']
        }]
    })

# Clean up: delete the created item and folder
aclib.RunTapirCommand ('DeleteKeynoteItems', {
        'keynoteItemIds': [{ 'keynoteItemId': newItemId }]
    })
aclib.RunTapirCommand ('DeleteKeynoteFolders', {
        'keynoteFolderIds': [{ 'keynoteFolderId': newFolderId }]
    })
