"""Reads the element default stored in Favorites through the Favorite manager (Archicad 29 or newer), without
applying it or placing an element: element type, layer, element ID, folder, library part and GDL parameters,
the marker of every element type that has one and the sub-elements with their kinds (subTypes). A Favorite
without a marker has no 'marker' field, and a Favorite that cannot be read gets an error item.
"""
import aclib

favoriteNames = []
for elementType in ['Window', 'CurtainWall', 'Stair']:
    favoriteNames += aclib.RunTapirCommand ('GetFavoritesByType', {
        'elementType': elementType
    }).get ('favorites', [])[:1]

response = aclib.RunTapirCommand ('GetFavoriteDefaults', {
    'favorites': favoriteNames
})

for favoriteDefault in response['favoriteDefaults']:
    if 'error' in favoriteDefault:
        continue
    print (favoriteDefault['favorite'], favoriteDefault['elementType'], 'on layer', favoriteDefault['layerIndex'],
           'with', len (favoriteDefault['subElements']), 'sub-elements')
    for subElement in favoriteDefault['subElements']:
        print ('    ', subElement['elementType'], subElement['subTypes'])
