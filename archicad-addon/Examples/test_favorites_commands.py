import aclib

allColumns = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Column'
    })['elements']

firstColumn = allColumns[0]

aclib.RunTapirCommand (
    'CreateFavoritesFromElements', {
        'favoritesFromElements': [{
            'elementId': firstColumn['elementId'],
            'favorite': 'ColumnFromPython'
        }]
    })

aclib.RunTapirCommand (
    'ApplyFavoritesToElementDefaults', {
        'favorites': ['ColumnFromPython']
    })

aclib.RunTapirCommand (
    'CreateColumns', {
        'columnsData': [
            {'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0}},
            {'coordinates': {'x': 1.0, 'y': 1.0, 'z': 1.0}}
        ]
    })