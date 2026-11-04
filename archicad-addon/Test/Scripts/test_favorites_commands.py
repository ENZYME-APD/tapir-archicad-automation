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
    'GetFavoritePreviewImage', {
        'favorite': 'ColumnFromPython'
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

allColumnsAfterCreate = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'Column'
    })['elements']

newColumn = allColumnsAfterCreate[-1]

aclib.RunTapirCommand (
    'CreateFavoritesFromElements', {
        'favoritesFromElements': [{
            'elementId': firstColumn['elementId'],
            'favorite': 'ZZTestFolderFavorite',
            'folder': ['ZZTest', 'Sub']
        }]
    })

aclib.RunTapirCommand (
    'ApplyFavoritesToElements', {
        'favoritesToApply': [{
            'elementId': newColumn['elementId'],
            'favorite': 'ColumnFromPython'
        }]
    })

allPolylines = aclib.RunTapirCommand (
    'GetElementsByType', {
        'elementType': 'PolyLine'
    })['elements']

aclib.RunTapirCommand (
    'ApplyFavoritesToElements', {
        'favoritesToApply': [{
            'elementId': allPolylines[0]['elementId'],
            'favorite': 'ColumnFromPython'
        }]
    })

aclib.RunTapirCommand (
    'ApplyFavoritesToElements', {
        'favoritesToApply': [{
            'elementId': newColumn['elementId'],
            'favorite': 'ColumnFromPython'
        }],
        'applySettings': False
    })

aclib.RunTapirCommand (
    'UpdateFavoritesFromElements', {
        'favoritesFromElements': [{
            'elementId': firstColumn['elementId'],
            'favorite': 'ColumnFromPython'
        }]
    })

aclib.RunTapirCommand (
    'RenameFavorites', {
        'renames': [{
            'oldName': 'ZZTestFolderFavorite',
            'newName': 'ZZTestFolderFavoriteRenamed'
        }]
    })

aclib.RunTapirCommand (
    'DeleteFavorites', {
        'favorites': ['ZZTestFolderFavoriteRenamed']
    })