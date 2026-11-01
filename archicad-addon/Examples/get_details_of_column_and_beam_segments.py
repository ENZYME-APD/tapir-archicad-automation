import aclib

columns = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Column'})['elements']
beams = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Beam'})['elements']

subelements = aclib.RunTapirCommand ('GetSubelementsOfHierarchicalElements', {'elements': columns + beams})['subelements']

segmentElements = []
for subelementsOfElement in subelements:
    for segment in subelementsOfElement.get ('columnSegments', []) + subelementsOfElement.get ('beamSegments', []):
        if 'elementId' in segment:
            segmentElements.append (segment)

detailsOfSegments = aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': segmentElements})['detailsOfElements']
