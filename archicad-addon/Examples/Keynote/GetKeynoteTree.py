"""Prints the keynote folders and items of the project as an indented tree, using GetKeynoteTree.

Requires Archicad 28 or later.
"""
import aclib

def PrintFolder (folder, indent):
    print ('  ' * indent + '[{}] {}'.format (folder['key'], folder['title']))
    for item in folder['items']:
        print ('  ' * (indent + 1) + '{} {}'.format (item['key'], item['title']))
    for subFolder in folder['subFolders']:
        PrintFolder (subFolder, indent + 1)

tree = aclib.RunTapirCommand ('GetKeynoteTree')
for folder in tree['foldersInRoot']:
    PrintFolder (folder, 0)
for item in tree['itemsInRoot']:
    print ('{} {}'.format (item['key'], item['title']))
