import math

import aclib

# A Drawing favorite made the Drawing tool's defaults (#652): Archicad does not give out the settings
# of a Drawing favorite as element settings, so ApplyFavoritesToElementDefaults applies them through
# the Favorite manager instead (Archicad 29 or newer). A Drawing placed afterwards without an angle
# takes the angle of the favorite.


def find_by_type(branch, navigator_item_type):
    for entry in branch:
        item = entry['navigatorItem']
        if item.get('type') == navigator_item_type:
            yield item
        children = item.get('children')
        if children:
            yield from find_by_type(children, navigator_item_type)


layout_book = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'LayoutBook'}})
layouts = list(find_by_type(layout_book['navigatorTree']['rootItem']['children'], 'LayoutItem'))
assert layouts, 'No layout found'
layout_db_id = aclib.RunTapirCommand('GetDatabaseIdFromNavigatorItemId', {
    'navigatorItemIds': [{'navigatorItemId': layouts[0]['navigatorItemId']}]
}, debug=False)['databases'][0]['databaseId']

project_map = aclib.RunCommand('API.GetNavigatorItemTree', {'navigatorTreeId': {'type': 'ProjectMap'}})
views = list(find_by_type(project_map['navigatorTree']['rootItem']['children'], 'StoryItem'))
assert views, 'No StoryItem found in ProjectMap'
nav_item_id = views[0]['navigatorItemId']


def create_drawing(**settings):
    drawing_data = {'navigatorItemId': nav_item_id, 'layoutDatabaseId': layout_db_id, 'position': {'x': 0.1, 'y': 0.1}}
    drawing_data.update(settings)
    return aclib.RunTapirCommand('CreateDrawings', {'drawingsData': [drawing_data]}, debug=False)['elements'][0]


# A Drawing turned by 30 degrees, captured as a favorite.
source = create_drawing(name='RotatedDrawing', angle=math.radians(30))
aclib.RunTapirCommand('CreateFavoritesFromElements', {
    'favoritesFromElements': [{'elementId': source['elementId'], 'favorite': 'Rotated Drawing'}]
})

print('Favorite details:', aclib.RunTapirCommand('GetFavoriteDetails', {'favorites': ['Rotated Drawing']}, debug=False))
aclib.RunTapirCommand('ApplyFavoritesToElementDefaults', {'favorites': ['Rotated Drawing']})

placed = create_drawing()
details = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': [placed]}, debug=False)['detailsOfElements'][0]['details']
print('Angle of the Drawing placed after applying the favorite: {:.1f} degrees'.format(math.degrees(details['angle'])))

aclib.RunTapirCommand('DeleteFavorites', {'favorites': ['Rotated Drawing']})
