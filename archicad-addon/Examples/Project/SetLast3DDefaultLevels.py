"""Sets the last 3D default levels of the wall and slab defaults with SetLast3DDefaultLevels, then restores them."""
import aclib

elementTypes = ['Wall', 'Slab']
originalLevels = aclib.RunTapirCommand ('GetLast3DDefaultLevels', {'elementTypes': elementTypes})['levels']

aclib.PrintJson (aclib.RunTapirCommand ('SetLast3DDefaultLevels', {
    'elementTypes': elementTypes,
    'levels': [1.0, 2.5]
}))
aclib.RunTapirCommand ('GetLast3DDefaultLevels', {'elementTypes': elementTypes})

aclib.PrintJson (aclib.RunTapirCommand ('SetLast3DDefaultLevels', {'elementTypes': elementTypes, 'levels': originalLevels}))
