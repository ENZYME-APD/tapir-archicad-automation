"""Lists every element of the project with GetAllElements, then only the ones visible in 3D."""
import aclib

elements = aclib.RunTapirCommand ('GetAllElements', {}, debug = False)['elements']
print ('{} elements in the project.'.format (len (elements)))

aclib.RunTapirCommand ('GetAllElements', {'filters': ['IsVisibleIn3D']})
