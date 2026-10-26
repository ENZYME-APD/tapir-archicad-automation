import aclib

# Requires Archicad 28 or newer; the routing node defaults need Archicad 29 and the
# electrical project settings Archicad 30.

# The 'defaults' parameter changes only what is placed, not the tool's settings.

# Place a pipe route whose elbows use the first pipe elbow preference table and a
# radius factor of 1.5.
pipeFittings = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
        'domain': 'Piping',
        'includeFittingTables': True
    })

elbowDefaults = { 'factorRadius': 1.5 }
if pipeFittings.get ('elbowTables'):
    elbowDefaults['preferenceTableId'] = pipeFittings['elbowTables'][0]['guid']

createdRoutes = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    { 'x': 0.0, 'y': -8.0, 'z': 1.0 },
                    { 'x': 4.0, 'y': -8.0, 'z': 1.0 },
                    { 'x': 4.0, 'y': -4.0, 'z': 1.0 }
                ],
                'defaults': {
                    'routingNode': {
                        'elbow': elbowDefaults
                    }
                }
            }
        ]
    })['elements']

# Place a pipe terminal with 0.02 m insulation and one without insulation.
createdElements = aclib.RunTapirCommand ('CreateMEPElements', {
        'elementsData': [
            {
                'type': 'Terminal',
                'domain': 'Piping',
                'position': { 'x': 6.0, 'y': -8.0, 'z': 1.0 },
                'defaults': { 'insulationThickness': 0.02 }
            },
            {
                'type': 'Terminal',
                'domain': 'Piping',
                'position': { 'x': 8.0, 'y': -8.0, 'z': 1.0 },
                'defaults': { 'removeInsulation': True }
            }
        ]
    })['elements']

# Replace every wiring type of the electrical project settings by one, then put the
# original wiring types back.
settings = aclib.RunTapirCommand ('GetElectricalProjectSettings')
if settings is not None and 'wiringTypes' in settings:
    aclib.RunTapirCommand ('ModifyElectricalProjectSettings', {
            'clearWiringTypes': True,
            'wiringTypesToAdd': [
                { 'name': 'Cu 2.5', 'crossSectionalArea': 2.5 }
            ]
        })
    aclib.RunTapirCommand ('GetElectricalProjectSettings')
    aclib.RunTapirCommand ('ModifyElectricalProjectSettings', {
            'clearWiringTypes': True,
            'wiringTypesToAdd': settings['wiringTypes']
        })

# Clean up
createdIds = [e['elementId'] for e in createdRoutes + createdElements if 'elementId' in e]
if createdIds:
    aclib.RunTapirCommand ('DeleteElements', {
            'elements': [{ 'elementId': elementId } for elementId in createdIds]
        })
