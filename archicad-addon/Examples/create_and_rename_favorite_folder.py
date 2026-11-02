import aclib

# Creates a folder in the root of the Favorites palette, a subfolder in it,
# then renames the subfolder (Archicad 29 or newer). Folders are given by their
# path, the folder names from the root down, as GetFavoriteFolders returns them.

parentName = 'Tapir Example Folder'

aclib.RunTapirCommand ('CreateFavoriteFolder', {
    'parentPath': [],
    'name': parentName
})

aclib.RunTapirCommand ('CreateFavoriteFolder', {
    'parentPath': [parentName],
    'name': 'Walls'
})

aclib.RunTapirCommand ('RenameFavoriteFolder', {
    'path': [parentName, 'Walls'],
    'newName': 'Exterior Walls'
})
