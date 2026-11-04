import aclib

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
columns = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Column'})['elements']

# Zooms the current window (2D or 3D) to the elements and keeps the selection as it was.
aclib.RunTapirCommand (
    'ZoomToElements', {
        'elements': walls + columns,
        'keepSelection': True
    })
