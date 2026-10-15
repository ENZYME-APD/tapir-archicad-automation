import aclib

# Requires Archicad 30 or newer.

# Add a wiring type and a load classification to the electrical project settings,
# unless they are already there (adding an existing name fails).
settings = aclib.RunTapirCommand ('GetElectricalProjectSettings')
settingsChanges = {}
if 'Cu 1.5' not in [wiringType['name'] for wiringType in settings['wiringTypes']]:
    settingsChanges['wiringTypesToAdd'] = [{'name': 'Cu 1.5', 'crossSectionalArea': 1.5}]
if 'Kitchen' not in [loadClassification['name'] for loadClassification in settings['loadClassifications']]:
    settingsChanges['loadClassificationsToAdd'] = [{'name': 'Kitchen', 'demandFactor': 0.7}]
if settingsChanges:
    aclib.RunTapirCommand ('ModifyElectricalProjectSettings', settingsChanges)

# Place a main panel and a sub-panel.
panels = aclib.RunTapirCommand ('CreateElectricalPanels', {
        'electricalPanelsData': [
            {'position': {'x': 0.0, 'y': 0.0, 'z': 1.0}, 'panelName': 'Main Panel', 'circuitNamePrefix': 'MP-'},
            {'position': {'x': 6.0, 'y': 0.0, 'z': 1.0}, 'panelName': 'Kitchen Panel', 'circuitNamePrefix': 'KP-'}
        ]
    })['elements']
mainPanelId = panels[0]['elementId']
kitchenPanelId = panels[1]['elementId']

# Add a lighting circuit and a three-phase feeder circuit to the main panel.
circuits = aclib.RunTapirCommand ('CreateElectricalCircuits', {
        'electricalCircuitsData': [
            {'electricalPanelId': mainPanelId, 'description': 'Lighting', 'voltage': 230, 'wiringTypeName': 'Cu 1.5'},
            {'electricalPanelId': mainPanelId, 'description': 'Kitchen feeder', 'voltage': 400,
             'isPhaseAUsed': True, 'isPhaseBUsed': True, 'isPhaseCUsed': True}
        ]
    })['elements']
lightingCircuitId = circuits[0]['elementId']
feederCircuitId = circuits[1]['elementId']

# Place two lights on the lighting circuit and a socket that is not connected yet.
devices = aclib.RunTapirCommand ('CreateElectricalDevices', {
        'electricalDevicesData': [
            {'position': {'x': 2.0, 'y': 3.0, 'z': 2.5}, 'apparentPower': 60, 'circuitId': lightingCircuitId},
            {'position': {'x': 4.0, 'y': 3.0, 'z': 2.5}, 'apparentPower': 60, 'circuitId': lightingCircuitId},
            {'position': {'x': 7.0, 'y': 3.0, 'z': 0.3}, 'apparentPower': 2000, 'loadClassificationName': 'Kitchen'}
        ]
    })['elements']
socketId = devices[2]['elementId']

# Feed the kitchen panel from the main panel, and put the socket on a new circuit of the kitchen panel.
kitchenCircuitId = aclib.RunTapirCommand ('CreateElectricalCircuits', {
        'electricalCircuitsData': [{'electricalPanelId': kitchenPanelId, 'description': 'Sockets'}]
    })['elements'][0]['elementId']
aclib.RunTapirCommand ('ConnectToElectricalCircuits', {
        'connectionsData': [
            {'elementId': kitchenPanelId, 'circuitId': feederCircuitId},
            {'elementId': socketId, 'circuitId': kitchenCircuitId}
        ]
    })

# Change a few values afterwards.
aclib.RunTapirCommand ('ModifyElectricalCircuits', {
        'electricalCircuitsData': [{'elementId': lightingCircuitId, 'description': 'Lighting ground floor'}]
    })
aclib.RunTapirCommand ('ModifyElectricalDevices', {
        'electricalDevicesData': [{'elementId': devices[0]['elementId'], 'apparentPower': 40}]
    })
aclib.RunTapirCommand ('ModifyElectricalPanels', {
        'electricalPanelsData': [{'elementId': mainPanelId, 'rotation': 0.0, 'anchorPoint': {'x': 0.0, 'y': 0.5, 'z': 1.0}}]
    })

# The connected load of the main panel now includes the kitchen panel.
mainPanel = aclib.RunTapirCommand ('GetElectricalPanels', {'elements': [{'elementId': mainPanelId}]})['electricalPanels'][0]
print (f"{mainPanel['panelName']}: connected load {mainPanel['connectedLoad']['total']}, demand load {mainPanel['demandLoad']['total']}")

# Remove the kitchen circuit again: the socket is disconnected.
aclib.RunTapirCommand ('DeleteElectricalCircuits', {'electricalCircuits': [{'elementId': kitchenCircuitId}]})
