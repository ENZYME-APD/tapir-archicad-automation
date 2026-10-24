import aclib

exportTolerance = aclib.RunTapirCommand ('GetExportTolerance')

elementTypes = ['Wall', 'Column', 'Slab']

# Store the original levels to restore them at the end
originalLevels = aclib.RunTapirCommand (
    'GetLast3DDefaultLevels', {
        'elementTypes': elementTypes
    })

aclib.RunTapirCommand (
    'SetLast3DDefaultLevels', {
        'elementTypes': elementTypes,
        'levels': [1.0, 2.0, 3.0]
    })

aclib.RunTapirCommand (
    'GetLast3DDefaultLevels', {
        'elementTypes': elementTypes
    })

aclib.RunTapirCommand (
    'SetLast3DDefaultLevels', {
        'elementTypes': elementTypes,
        'levels': originalLevels['levels']
    }, debug = False)
