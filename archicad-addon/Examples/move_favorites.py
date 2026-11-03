import aclib

# Moves the Favorites in the root of the Favorites palette into a new folder
# (Archicad 29 or newer). Folders are given by their path, the folder names
# from the root down, as GetFavoriteFolders returns them.
# The command answers with an ExecutionResult for each Favorite.

folderName = 'Tapir Example Moved Favorites'

rootFolder = aclib.RunTapirCommand ('GetFavoriteFolders', {})['rootFolder']
favorites = rootFolder['favorites']

aclib.RunTapirCommand ('CreateFavoriteFolder', {
    'parentPath': [],
    'name': folderName
})

aclib.RunTapirCommand ('MoveFavorites', {
    'favorites': favorites,
    'targetPath': [folderName]
})
