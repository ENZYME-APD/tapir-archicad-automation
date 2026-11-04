"""Lists the walls of the project with GetElementsByType: all of them, then only the editable ones on the current story."""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements']
print ('{} walls in the project.'.format (len (walls)))

aclib.PrintJson (aclib.RunTapirCommand ('GetElementsByType', {
    'elementType': 'Wall',
    'filters': ['IsEditable', 'OnActualFloor']
}))
