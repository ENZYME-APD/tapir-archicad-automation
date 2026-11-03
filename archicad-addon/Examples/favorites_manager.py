import os
import tempfile
import aclib

# The Favorite commands that use the Favorite manager of Archicad 29: create and change a
# Favorite from another Favorite or from the Opening tool default, apply a Favorite to its
# tool, import Favorite files exported with the xml format, and move a Favorites folder.

allFavorites = aclib.RunTapirCommand ('GetAllFavorites')
if not allFavorites or not allFavorites['favorites']:
    print ('The project has no Favorites, or the Archicad version is older than 29.')
    exit ()
sourceFavorite = allFavorites['favorites'][0]['name']

# A copy of the first Favorite in the root folder. RenameNew picks a free name if it is taken.
created = aclib.RunTapirCommand ('CreateFavorite', {
    'name': 'Tapir Example Favorite',
    'source': {'favoriteName': sourceFavorite},
    'conflictPolicy': 'RenameNew'
})
if not created or 'name' not in created:
    exit ()
copyName = created['name']

aclib.RunTapirCommand ('ChangeFavorite', {
    'favoriteName': copyName,
    'source': {'favoriteName': sourceFavorite}
})

aclib.RunTapirCommand ('ApplyFavoriteToToolDefault', {
    'favoriteName': copyName
})

# A Favorite from the current settings of the Opening tool.
openingFavorite = aclib.RunTapirCommand ('CreateFavorite', {
    'name': 'Tapir Example Opening',
    'source': {'toolDefault': 'Opening'},
    'conflictPolicy': 'OverwriteExisting'
})

# Export the copy as a Favorite file, delete it, and import it again.
xmlFolder = os.path.join (tempfile.gettempdir (), 'tapir_example_favorites_manager')
os.makedirs (xmlFolder, exist_ok=True)
aclib.RunTapirCommand ('ExportFavorites', {
    'path': xmlFolder,
    'names': [copyName],
    'format': 'xml'
})
aclib.RunTapirCommand ('DeleteFavorites', {'favorites': [copyName]})
aclib.RunTapirCommand ('ImportFavorites', {
    'path': xmlFolder,
    'format': 'xml',
    'conflictPolicy': 'Skip'
})

# Moving a folder needs two folders of the Favorites palette; set their paths to try it.
folderToMove = []
targetFolder = []
if folderToMove:
    aclib.RunTapirCommand ('MoveFavoriteFolder', {
        'folder': folderToMove,
        'targetFolder': targetFolder
    })

namesToDelete = [copyName]
if openingFavorite and 'name' in openingFavorite:
    namesToDelete.append (openingFavorite['name'])
aclib.RunTapirCommand ('DeleteFavorites', {'favorites': namesToDelete})
