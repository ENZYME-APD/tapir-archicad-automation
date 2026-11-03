import aclib

# Swapping a placeholder window for the final one: a Favorite is applied to a window whose
# size, sill height and Element ID are already right, and keepFields copies exactly those back
# after the Favorite has been applied (#903). The Favorite is captured from a window of a
# different size, so without keepFields the target would take over that size.

wallId = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{
        'begCoordinate': { 'x': 100.0, 'y': 0.0 },
        'endCoordinate': { 'x': 110.0, 'y': 0.0 },
        'height': 3.0,
        'thickness': 0.3
    }]
})['elements'][0]['elementId']

windows = aclib.RunTapirCommand ('CreateWindows', {
    'windowsData': [
        { 'ownerWallId': wallId, 'centerOffset': 2.0, 'width': 0.6, 'height': 0.6, 'sillHeight': 1.5 },
        { 'ownerWallId': wallId, 'centerOffset': 6.0, 'width': 1.2, 'height': 1.4, 'sillHeight': 0.9 }
    ]
})['elements']
sourceWindow = windows[0]
targetWindow = windows[1]

elementIdPropertyId = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [{ 'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID' }]
})['properties'][0]['propertyId']

aclib.RunTapirCommand ('SetPropertyValuesOfElements', {
    'elementPropertyValues': [
        { 'elementId': sourceWindow['elementId'], 'propertyId': elementIdPropertyId, 'propertyValue': { 'value': 'PLACEHOLDER' } },
        { 'elementId': targetWindow['elementId'], 'propertyId': elementIdPropertyId, 'propertyValue': { 'value': 'F-007' } }
    ]
})

aclib.RunTapirCommand ('CreateFavoritesFromElements', {
    'favoritesFromElements': [{
        'elementId': sourceWindow['elementId'],
        'favorite': 'SmallWindowFromPython'
    }]
})

def PrintTargetWindow (label):
    detailsOfElement = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': [targetWindow],
        'fields': ['id', 'details']
    }, debug = False)['detailsOfElements'][0]
    details = detailsOfElement['details']
    print ('{}: id={} width={:.2f} height={:.2f} sillHeight={:.2f}'.format (
        label, detailsOfElement['id'], details['width'], details['height'], details['sillHeight']))

PrintTargetWindow ('Before')

# The Favorite's library part and settings are applied; the four listed fields stay as they were.
aclib.RunTapirCommand ('ApplyFavoritesToElements', {
    'favoritesToApply': [{
        'elementId': targetWindow['elementId'],
        'favorite': 'SmallWindowFromPython'
    }],
    'keepFields': ['id', 'width', 'height', 'sillHeight']
})

PrintTargetWindow ('After')

aclib.RunTapirCommand ('DeleteFavorites', {
    'favorites': ['SmallWindowFromPython']
})

aclib.RunTapirCommand ('DeleteElements', {
    'elements': [{ 'elementId': wallId }]
})
