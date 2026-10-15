import aclib

# Requires Archicad 30 or newer.

# The voltage, wiring types and load classifications set in the electrical project settings.
settings = aclib.RunTapirCommand ('GetElectricalProjectSettings')
print (f"Project voltage: {settings['voltage']}")
for wiringType in settings['wiringTypes']:
    print (f"  Wiring type {wiringType['name']}: cross section {wiringType['crossSectionalArea']}")
for loadClassification in settings['loadClassifications']:
    print (f"  Load classification {loadClassification['name']}: demand factor {loadClassification['demandFactor']}")

# Every electrical panel of the project, with its connected and demand loads.
panels = aclib.RunTapirCommand ('GetElectricalPanels')['electricalPanels']
for panel in panels:
    if 'error' in panel:
        continue
    print (f"Panel {panel['panelName']}: {len (panel['circuits'])} circuits, "
           f"connected load {panel['connectedLoad']['total']}, demand load {panel['demandLoad']['total']}")

# The circuits of the panels, with the devices connected to them.
circuits = aclib.RunTapirCommand ('GetElectricalCircuits', {
        'electricalPanels': [{'elementId': panel['elementId']} for panel in panels if 'error' not in panel]
    })['electricalCircuits']
for circuit in circuits:
    if 'error' in circuit:
        continue
    print (f"Circuit {circuit['name']} ({circuit['phases']}, {circuit['voltage']} V, wiring '{circuit['wiringTypeName']}'): "
           f"{len (circuit['childDevices'])} devices, connected load {circuit['connectedLoad']['total']}, "
           f"estimated cable length {circuit['estimatedCableLength']}")

# Every electrical device of the project, with the circuit it is connected to.
devices = aclib.RunTapirCommand ('GetElectricalDevices')['electricalDevices']
for device in devices:
    if 'error' in device:
        continue
    parentCircuit = device['parentCircuitId']['guid'] if 'parentCircuitId' in device else 'not connected'
    print (f"Device {device['elementId']['guid']}: apparent power {device['apparentPower']} VA, "
           f"load classification '{device['loadClassificationName']}', circuit {parentCircuit}")
