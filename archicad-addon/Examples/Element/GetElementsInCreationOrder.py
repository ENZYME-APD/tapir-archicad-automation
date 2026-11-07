"""Lists the elements of the current database in the order they were created, with GetElementsInCreationOrder."""
import aclib

elements = aclib.RunTapirCommand ('GetElementsInCreationOrder', {})['elements']
if not elements:
    raise SystemExit ('No element in the current database.')

print ('First created element: {}'.format (elements[0]['elementId']['guid']))
print ('Last created element: {}'.format (elements[-1]['elementId']['guid']))
