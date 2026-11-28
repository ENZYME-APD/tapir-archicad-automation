"""Reads the outline of a stair (its left and right boundaries and its walking lines) and moves it the way Archicad allows.

Archicad lays out the boundaries and the walking lines of a stair itself, from its baseline and its
flight width, and the Archicad API cannot write them: CreateStairs, ModifyStairs and RecreateStairs
refuse an item that gives one (leftBoundary, rightBoundary, walkingLine, customWalkingLine,
centerWalkingLine) and say what to change instead. This script creates a straight stair, prints its
outline, shows that ModifyStairs refuses a custom left boundary, widens the flight with flightWidth,
which moves both boundaries and keeps the walking line in the middle, prints the outline again, then
deletes the stair it created.
"""
import aclib

aclib.raiseOnError = True


def PrintOutline (title, stair):
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': [stair]})['detailsOfElements'][0]['details']
    print (title + ' (flight width {:.2f} m):'.format (details['flightWidth']))
    for key in ('leftBoundary', 'rightBoundary', 'walkingLine', 'customWalkingLine', 'centerWalkingLine'):
        if key in details:
            points = ', '.join ('({:.2f}, {:.2f})'.format (p['x'], p['y']) for p in details[key]['points'])
            print ('  {:18} {}'.format (key, points))
    return details


stair = aclib.RunTapirCommand ('CreateStairs', {'stairsData': [{
    'baseLinePoints': [{'x': 0.0, 'y': 0.0}, {'x': 4.0, 'y': 0.0}],
    'baselinePosition': 'Center',
    'zCoordinate': 0.0,
    'totalHeight': 2.8,
    'flightWidth': 1.2
}]})['elements'][0]

details = PrintOutline ('Created', stair)

# A boundary cannot be written: the item is refused and the stair is left as it was.
leftBoundary = dict (details['leftBoundary'])
leftBoundary['points'] = [{'x': p['x'], 'y': p['y'] + 0.3} for p in leftBoundary['points']]
result = aclib.RunTapirCommand ('ModifyStairs', {'stairsWithDetails': [{
    'elementId': stair['elementId'],
    'leftBoundary': leftBoundary
}]})['executionResults'][0]
print ('ModifyStairs with leftBoundary: ' + result['error']['message'] if not result['success'] else 'ModifyStairs with leftBoundary: written')

# What moves the boundaries instead: the flight width (and the baseline, with RecreateStairs).
aclib.RunTapirCommand ('ModifyStairs', {'stairsWithDetails': [{'elementId': stair['elementId'], 'flightWidth': 1.8}]})
PrintOutline ('After flightWidth 1.8', stair)

aclib.RunTapirCommand ('DeleteElements', {'elements': [stair]})
