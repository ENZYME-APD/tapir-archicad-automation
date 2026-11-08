"""Prints the Layout Book as an indented tree with GetNavigatorItemTree.

The other maps are ProjectMap, PublicViewMap and PublisherSets (with publisherSetName).
"""
import aclib

def PrintTree (item, indent):
    print ('  ' * indent + '{} {} ({})'.format (item['uiId'], item['name'], item['type']))
    for child in item.get ('children', []):
        PrintTree (child['navigatorItem'], indent + 1)

tree = aclib.RunTapirCommand ('GetNavigatorItemTree', {'navigatorMapId': 'LayoutBook'})['navigatorItemTree']
PrintTree (tree, 0)
