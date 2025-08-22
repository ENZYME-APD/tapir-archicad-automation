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
    'GetFavoritesByType', {
        'elementType': 'Column'
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

aclib.RunTapirCommand (
    'CreatePolylines', {
        'polylinesData': [
            {'coordinates': [
                {'x': 0.0, 'y': 0.0},
                {'x': 2.0, 'y': 2.0},
                {'x': 3.0, 'y': 2.0}
            ],
            'arcs': [
                {'begIndex': 0, 'endIndex': 1, 'arcAngle': 3.14}
            ]
            },
            {'coordinates': [
                {'x': 1.0, 'y': 0.0},
                {'x': 5.0, 'y': 3.1}
            ]}
        ]
    })