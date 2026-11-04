"""Requires Archicad 27 or newer. On Archicad 27 this example reads the tool defaults and works on the selected
MEP elements (select a flexible segment and a terminal first), because Archicad 27 has no commands
to place them. From Archicad 28 it places its own elements and deletes them at the end.
"""
import sys
import aclib

# GetMEPToolDefaults: the rigid segment and branch defaults of the routing tool, and from Archicad 30
# the Electrical Panel tool with its number of circuit defaults.
defaults = aclib.RunTapirCommand ('GetMEPToolDefaults', {'domains': ['Ventilation', 'Piping']})
if defaults is None or 'error' in defaults:
    print ('Could not read the MEP tool defaults, this example needs Archicad 27 or newer.')
    sys.exit ()
for domainDefaults in defaults['toolDefaults']:
    print (f"{domainDefaults['domain']}: rigid segment {domainDefaults.get ('rigidSegment')}, "
           f"branch with {len (domainDefaults.get ('branch', {}).get ('ports', []))} port(s)")
    terminal = domainDefaults.get ('terminal')
    if terminal is not None:
        print (f"  Terminal port shapes: {[port.get ('shape') for port in terminal['ports']]}")
if 'electricalPanel' in defaults:
    print (f"Electrical panel: {defaults['electricalPanel']['circuitDefaultCount']} circuit default(s)")

def ModifyAndReport (title, items):
    result = aclib.RunTapirCommand ('ModifyMEPElements', {'elements': items})
    if result is None or 'error' in result:
        print (f"{title}: {result['error']['message'] if result else 'failed'}")
        return
    for executionResult in result['executionResults']:
        print (f"{title}: {'succeeded' if executionResult['success'] else executionResult['error']['message']}")

placedIds = []
terminal = aclib.RunTapirCommand ('CreateMEPElements', {
        'elementsData': [
            {'type': 'Terminal', 'domain': 'Ventilation', 'position': {'x': 0.0, 'y': -40.0, 'z': 2.5}}
        ]
    })

if terminal is None or 'error' in terminal:
    # Archicad 27: the selected elements.
    selected = aclib.RunTapirCommand ('GetSelectedElements', {})
    selectedIds = [{'elementId': e['elementId']} for e in (selected or {}).get ('elements', [])]
    ports = aclib.RunTapirCommand ('GetMEPPorts', {'elements': selectedIds})
    for elementPorts in (ports or {}).get ('elementPorts', []):
        if 'error' in elementPorts:
            print (f"Ports: {elementPorts['error']['message']}")
            continue
        print (f"Ports: {[(port['domain'], port['shape'], port.get ('isConnected')) for port in elementPorts['ports']]}")
    # Each item fails on the elements its fields do not belong to.
    ModifyAndReport ('Flexible segment width 0.3', [dict (item, width = 0.3) for item in selectedIds])
    sys.exit ()

# Archicad 28 or newer: move the terminal and read its ports.
terminalId = terminal['elements'][0].get ('elementId')
if terminalId is not None:
    placedIds.append (terminalId)
    ModifyAndReport ('Move the terminal', [{'elementId': terminalId, 'anchorPoint': {'x': 1.0, 'y': -40.0, 'z': 2.8}}])
    ports = aclib.RunTapirCommand ('GetMEPPorts', {'elements': [{'elementId': terminalId}]})
    for port in ports['elementPorts'][0].get ('ports', []):
        print (f"Terminal port {port['portId']}: {port['shape']} at {port['position']}")

# A ventilation flexible segment through control points, then new control points and a new width.
flexible = aclib.RunTapirCommand ('PlaceMEPFlexibleSegments', {
        'flexibleSegmentsData': [
            {
                'domain': 'Ventilation',
                'controlPoints': [
                    {'x': 0.0, 'y': -42.0, 'z': 2.5},
                    {'x': 1.5, 'y': -42.5, 'z': 2.5},
                    {'x': 3.0, 'y': -42.0, 'z': 2.5}
                ]
            }
        ]
    })['elements'][0]
if 'elementId' in flexible:
    placedIds.append (flexible['elementId'])
    ModifyAndReport ('New control points and width', [{
            'elementId': flexible['elementId'],
            'controlPoints': [
                {'x': 0.0, 'y': -42.0, 'z': 2.5},
                {'x': 1.5, 'y': -43.0, 'z': 2.5},
                {'x': 3.0, 'y': -42.0, 'z': 2.5}
            ],
            'width': 0.3
        }])
else:
    print (f"Flexible segment: {flexible['error']['message']}")

# A piping flexible segment along a polyline: a line, a spline and a line that connect at their ends.
polyline = [
    {'type': 'Line', 'begin': {'x': 0.0, 'y': -45.0, 'z': 1.0}, 'end': {'x': 1.0, 'y': -45.0, 'z': 1.0}},
    {'type': 'Spline', 'controlPoints': [
        {'x': 1.0, 'y': -45.0, 'z': 1.0},
        {'x': 1.5, 'y': -45.5, 'z': 1.0},
        {'x': 2.0, 'y': -46.0, 'z': 1.0}
    ]},
    {'type': 'Line', 'begin': {'x': 2.0, 'y': -46.0, 'z': 1.0}, 'end': {'x': 2.0, 'y': -47.0, 'z': 1.0}}
]
pipe = aclib.RunTapirCommand ('PlaceMEPFlexibleSegments', {
        'flexibleSegmentsData': [{'domain': 'Piping', 'polyline': polyline}]
    })['elements'][0]
if 'elementId' in pipe:
    placedIds.append (pipe['elementId'])
    details = aclib.RunTapirCommand ('GetMEPElementDetails', {'elements': [{'elementId': pipe['elementId']}]})
    print (f"Polyline parts: {[part['type'] for part in details['elementDetails'][0].get ('polyline', [])]}")
    # Move the end of the last line.
    polyline[2]['end'] = {'x': 2.0, 'y': -48.0, 'z': 1.0}
    ModifyAndReport ('New polyline', [{'elementId': pipe['elementId'], 'polyline': polyline}])
else:
    print (f"Polyline flexible segment: {pipe['error']['message']}")

# Clean up
aclib.RunTapirCommand ('DeleteElements', {'elements': [{'elementId': elementId} for elementId in placedIds]})
