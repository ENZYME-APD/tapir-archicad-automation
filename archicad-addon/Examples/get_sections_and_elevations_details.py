import aclib

# Reads every setting of the sections and elevations of the project.
sections = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'CutPlane'})['elements']
elevations = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Elevation'})['elements']

details = aclib.RunTapirCommand ('GetSectionsAndElevationsDetails', {
    'elements': sections + elevations
})['detailsOfElements']

for detail in details:
    print (detail.get ('referenceId'), detail.get ('name'), detail.get ('horizontalRange'), detail.get ('verticalRange'))
