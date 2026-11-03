import aclib

# A column with a 10 cm insulation veneer: type, thickness and the building material of the
# veneer are applied to all segments of the column.
buildingMaterials = aclib.RunTapirCommand ('GetAttributesByType', {
    'attributeType': 'BuildingMaterial'
})['attributes']
veneerMaterial = buildingMaterials[-1]['attributeId']

columns = aclib.RunTapirCommand ('CreateColumns', {
    'columnsData': [{
        'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0},
        'height': 3.0,
        'width': 0.3,
        'depth': 0.3,
        'veneer': {
            'type': 'Other',
            'thickness': 0.1,
            'buildingMaterialId': veneerMaterial
        }
    }]
})['elements']

def PrintVeneers ():
    details = aclib.RunTapirCommand ('GetDetailsOfElements', {
        'elements': columns
    })['detailsOfElements']
    for column in details:
        for segment in column['details']['segments']:
            veneer = segment['veneer']
            print ('{} {:.2f}'.format (veneer['type'], veneer['thickness']))

PrintVeneers ()

# ModifyColumns changes only the given veneer fields; thickness 0 removes the veneer.
aclib.RunTapirCommand ('ModifyColumns', {
    'columnsWithDetails': [{
        'elementId': columns[0]['elementId'],
        'veneer': {'type': 'Finish', 'thickness': 0.05}
    }]
})
PrintVeneers ()

aclib.RunTapirCommand ('ModifyColumns', {
    'columnsWithDetails': [{
        'elementId': columns[0]['elementId'],
        'veneer': {'thickness': 0.0}
    }]
})
PrintVeneers ()

aclib.RunTapirCommand ('DeleteElements', {
    'elements': columns
})
