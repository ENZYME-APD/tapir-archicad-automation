import aclib

# Lists every Favorite of the Favorites palette (Archicad 29 or newer) with its folder path,
# classifications, element category values and property values. Give 'folderPath', the folder
# names from the root down, to read only one folder and its subfolders.

aclib.RunTapirCommand ('GetFavoriteContents', {})
