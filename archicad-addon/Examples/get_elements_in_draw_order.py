import aclib

print ('-' * 30)
elementsInDrawOrder = aclib.RunTapirCommand ('GetElementsInDrawOrder', {}, debug=False)['elements']
allElements = aclib.RunTapirCommand ('GetAllElements', {}, debug=False)['elements']
print ('Elements in drawing order = {}'.format (len (elementsInDrawOrder)))
print ('All elements = {}'.format (len (allElements)))
print ('-' * 30)

# The types and draw indices of the elements, in the order Archicad draws them.
detailsOfElements = aclib.RunTapirCommand ('GetDetailsOfElements', {
    'elements': elementsInDrawOrder,
    'fields': ['type', 'drawIndex']
}, debug=False)['detailsOfElements']
for index, detailsOfElement in enumerate (detailsOfElements):
    print ('{} {} drawIndex={}'.format (index, detailsOfElement.get ('type'), detailsOfElement.get ('drawIndex')))
print ('-' * 30)

# Only the fills on visible layers, still in drawing order.
fillsInDrawOrder = aclib.RunTapirCommand ('GetElementsInDrawOrder', {
    'elementType': 'Hatch',
    'filters': ['IsVisibleByLayer']
}, debug=False)['elements']
print ('Visible fills in drawing order = {}'.format (len (fillsInDrawOrder)))
