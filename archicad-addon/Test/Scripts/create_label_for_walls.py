from archicad import ACConnection
import aclib

conn = ACConnection.connect()
assert conn

acc = conn.commands
act = conn.types
acu = conn.utilities

propertyIds = acc.GetPropertyIds([
    act.BuiltInPropertyUserId("General_ElementID"),
    act.BuiltInPropertyUserId("General_Height"),
    act.BuiltInPropertyUserId("General_Width"),
    act.BuiltInPropertyUserId("General_Thickness")
])

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

aclib.RunTapirCommand (
    'CreateLabels', {
        'labelsData': [
            {
                'parentElementId': wall['elementId'],
                'text':
                    f'ElemID: <PROPERTY-{propertyIds[0].propertyId.guid}> \n' +
                    f'Height: <PROPERTY-{propertyIds[1].propertyId.guid}> \n' +
                    f'Width: <PROPERTY-{propertyIds[2].propertyId.guid}> \n' +
                    f'Thickness: <PROPERTY-{propertyIds[3].propertyId.guid}>'
            } for wall in walls
        ]
    })