import aclib

# Places MEP terminals with another library part, MEP system and port size than the
# tool's settings. Requires Archicad 28 or newer; xSize and ySize need Archicad 30.
# These fields change only what is placed, not the tool's settings.

# The library part by its name in the loaded libraries. Replace it with an MEP terminal
# library part loaded in your project.
terminalName = 'Air Terminal'
libraryParts = aclib.RunTapirCommand ('GetAvailableLibraryParts', {}, debug = False)['libraryParts']
terminalGuid = next ((part['guid'] for part in libraryParts if part['documentName'] == terminalName), None)

# The MEP system attribute "Supply Air".
mepSystems = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'MEPSystem'})['attributes']
supplyAirId = next ((attribute['attributeId'] for attribute in mepSystems if attribute['name'] == 'Supply Air'), None)

terminal = {
    'type': 'Terminal',
    'domain': 'Ventilation',
    'position': { 'x': 0.0, 'y': -12.0, 'z': 2.7 },
    'orientationDirection': { 'x': 0.0, 'y': 0.0, 'z': -1.0 },
    'orientationRotation': { 'x': 1.0, 'y': 0.0, 'z': 0.0 },
    # The first port becomes a 0.3 x 0.2 m rectangular connection.
    'ports': [
        { 'portIndex': 0, 'shape': 'Rectangular', 'width': 0.3, 'height': 0.2 }
    ]
}
if terminalGuid is not None:
    terminal['libraryPartId'] = { 'guid': terminalGuid }
if supplyAirId is not None:
    # Every ventilation port of the terminal gets this MEP system.
    terminal['mepSystemId'] = supplyAirId

createdElements = aclib.RunTapirCommand ('CreateMEPElements', {
        'elementsData': [terminal]
    })['elements']

createdIds = [e['elementId'] for e in createdElements if 'elementId' in e]

# The ports show the MEP system and the size.
if createdIds:
    aclib.RunTapirCommand ('GetMEPPorts', {
            'elements': [{ 'elementId': elementId } for elementId in createdIds]
        })

# Clean up
if createdIds:
    aclib.RunTapirCommand ('DeleteElements', {
            'elements': [{ 'elementId': elementId } for elementId in createdIds]
        })
