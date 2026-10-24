import aclib

# Pixel positions are local to the current Archicad view window and depend on its
# size, drawing scale and zoom, so the output differs from machine to machine.

response = aclib.RunTapirCommand ('ModelToScreen', {
    'coordinates': [
        {'x': 0.0, 'y': 0.0, 'z': 0.0},
        {'x': 5.0, 'y': 3.0, 'z': 0.0}
    ]
})

# ModelToScreen needs Archicad 29 or later; earlier versions return an error.
screenPoints = [point for point in (response or {}).get ('screenPoints', []) if 'error' not in point]
if screenPoints:
    aclib.RunTapirCommand ('ScreenToModel', {'screenPoints': screenPoints})
