import aclib

# Creates a folder with a subfolder in the root of the Favorites palette, then
# deletes it (Archicad 29 or newer). Archicad deletes a folder together with its
# Favorites and subfolders, so a folder that is not empty is deleted only with
# 'recursive': True; without it the command returns an error and deletes nothing.

folderName = 'Tapir Example Folder To Delete'

aclib.RunTapirCommand ('CreateFavoriteFolder', {
    'parentPath': [],
    'name': folderName
})

aclib.RunTapirCommand ('CreateFavoriteFolder', {
    'parentPath': [folderName],
    'name': 'Subfolder'
})

# Fails: the folder is not empty.
aclib.RunTapirCommand ('DeleteFavoriteFolder', {
    'path': [folderName]
})

aclib.RunTapirCommand ('DeleteFavoriteFolder', {
    'path': [folderName],
    'recursive': True
})
