import aclib

# Lists the folder tree of the Favorites palette (Archicad 29 or newer).
# Every folder has its name, its path from the root (the root's is empty),
# the names of the Favorites directly in it and its subfolders.

rootFolder = aclib.RunTapirCommand ('GetFavoriteFolders', {})['rootFolder']

def PrintFolder (folder, depth):
    indent = '    ' * depth
    print (indent + '/'.join (folder['path']) + '/')
    for favorite in folder['favorites']:
        print (indent + '    ' + favorite)
    for subFolder in folder['subFolders']:
        PrintFolder (subFolder, depth + 1)

PrintFolder (rootFolder, 0)
