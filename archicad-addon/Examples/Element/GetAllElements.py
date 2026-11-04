"""Lists every element of the project with GetAllElements, then only the ones visible in 3D."""
import aclib

elements = aclib.RunTapirCommand ('GetAllElements', {})['elements']
print ('{} elements in the project.'.format (len (elements)))

aclib.PrintJson (aclib.RunTapirCommand ('GetAllElements', {'filters': ['IsVisibleIn3D']}))
