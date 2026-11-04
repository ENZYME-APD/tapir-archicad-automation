"""Creates a wall, selects only it and deletes it with DeleteSelectedElements, the same as pressing the Delete key.

The original selection is restored at the end. Run it with the floor plan open.
"""
import aclib

originalSelection = aclib.RunTapirCommand ('GetSelectedElements', {})['elements']
walls = aclib.RunTapirCommand ('CreateWalls', {
    'wallsData': [{'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 5.0, 'y': 0.0}, 'height': 3.0}]
})['elements']
aclib.RunTapirCommand ('DeselectAll', {})
aclib.RunTapirCommand ('ChangeSelectionOfElements', {'addElementsToSelection': walls})

aclib.PrintJson (aclib.RunTapirCommand ('DeleteSelectedElements', {}))

aclib.RunTapirCommand ('ChangeSelectionOfElements', {'addElementsToSelection': originalSelection})
