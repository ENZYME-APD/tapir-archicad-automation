import sys
import aclib

# Requires Archicad 28 or newer; the connection style and the nominal size require Archicad 30.

# A pipe route and a duct route, then ModifyMEPPorts changes the flow direction of their first
# ports and the outer diameter of the pipe's first port.
created = aclib.RunTapirCommand ('CreateMEPRoutingElements', {
        'routingElementsData': [
            {
                'domain': 'Piping',
                'nodeCoordinates': [
                    {'x': 0.0, 'y': -15.0, 'z': 1.0},
                    {'x': 4.0, 'y': -15.0, 'z': 1.0}
                ]
            },
            {
                'domain': 'Ventilation',
                'nodeCoordinates': [
                    {'x': 0.0, 'y': -17.0, 'z': 3.0},
                    {'x': 4.0, 'y': -17.0, 'z': 3.0}
                ]
            }
        ]
    })
if created is None or 'error' in created or any ('error' in e for e in created['elements']):
    print ('Could not create the routing elements, this example needs Archicad 28 or newer.')
    sys.exit ()
routeIds = [e['elementId'] for e in created['elements']]

def GetFirstPorts ():
    elementPorts = aclib.RunTapirCommand ('GetMEPPorts', {
            'elements': [{'elementId': routeId} for routeId in routeIds]
        })['elementPorts']
    return [portsOfElement['ports'][0] for portsOfElement in elementPorts]

def PrintPorts (title, ports):
    print (title)
    for port in ports:
        print (f"  {port['domain']} port {port['name']}: shape {port['shape']}, "
               f"flow direction {port.get ('flowDirection', 'not reported')}, "
               f"connection style {port.get ('connectionStyle', 'not reported (Archicad 28-29)')}, "
               f"nominal size {port.get ('nominalSize', 'not reported')}")

ports = GetFirstPorts ()
PrintPorts ('Before:', ports)

pipePort = next (port for port in ports if port['domain'] == 'Piping')
ductPort = next (port for port in ports if port['domain'] == 'Ventilation')
results = aclib.RunTapirCommand ('ModifyMEPPorts', {
        'portsData': [
            {
                'portId': pipePort['portId'],
                'flowDirection': 'FlowIn',
                'outerDiameter': 0.05
            },
            {
                'portId': ductPort['portId'],
                'flowDirection': 'FlowOut'
            }
        ]
    })['executionResults']
for port, result in zip ([pipePort, ductPort], results):
    print (f"Modify {port['domain']} port: {'succeeded' if result['success'] else result['error']['message']}")

PrintPorts ('After:', GetFirstPorts ())

# Clean up
aclib.RunTapirCommand ('DeleteElements', {
        'elements': [{'elementId': routeId} for routeId in routeIds]
    })
