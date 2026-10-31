import aclib

# ShowElementsIn3D opens the 3D window on the given elements, as Show Selection in 3D
# does: they become the selection and only they are shown.
walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'}, debug = False)['elements']

if walls:
    aclib.RunTapirCommand ('ShowElementsIn3D', {'elements': walls[:2]})
    print ('Selected elements shown in 3D: {}'.format (len (aclib.RunTapirCommand ('GetSelectedElements', debug = False)['elements'])))
else:
    print ('No walls in the project; nothing to show selectively.')

# Without elements it shows every element of the project, as Show All in 3D does.
aclib.RunTapirCommand ('ShowElementsIn3D')
