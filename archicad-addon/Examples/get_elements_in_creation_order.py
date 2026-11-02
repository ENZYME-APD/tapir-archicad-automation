import aclib

elements = aclib.RunTapirCommand ('GetElementsInCreationOrder', {})['elements']

if elements:
    print ('First created element: ' + elements[0]['elementId']['guid'])
    print ('Last created element: ' + elements[-1]['elementId']['guid'])
