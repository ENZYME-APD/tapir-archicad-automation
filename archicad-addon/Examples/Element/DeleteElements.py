"""Creates a wall and deletes it again with DeleteElements."""
import aclib

walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 5.0, 'y': 0.0}, 'height': 3.0}]
})['elements']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteElements', {'elements': walls}))
