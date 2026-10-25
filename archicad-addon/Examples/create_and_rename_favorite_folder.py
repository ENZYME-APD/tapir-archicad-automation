import aclib

# Creates a folder in the root of the Favorites palette, a subfolder in it,
# then renames the subfolder (Archicad 29 or newer). Folders are given by their
# path, the folder names from the root down, as GetFavoriteFolders returns them.
# A folder name must be unique within its parent, so the steps a previous run
# already did are skipped.

parentName = 'Tapir Example Folder'

rootFolder = aclib.RunTapirCommand ('GetFavoriteFolders', {})['rootFolder']
parentFolder = next ((folder for folder in rootFolder['subFolders'] if folder['name'] == parentName), None)

if parentFolder is None:
    aclib.RunTapirCommand ('CreateFavoriteFolder', {
        'parentPath': [],
        'name': parentName
    })
    subFolderNames = []
else:
    subFolderNames = [folder['name'] for folder in parentFolder['subFolders']]

if 'Exterior Walls' not in subFolderNames:
    if 'Walls' not in subFolderNames:
        aclib.RunTapirCommand ('CreateFavoriteFolder', {
            'parentPath': [parentName],
            'name': 'Walls'
        })

    aclib.RunTapirCommand ('RenameFavoriteFolder', {
        'path': [parentName, 'Walls'],
        'newName': 'Exterior Walls'
    })
