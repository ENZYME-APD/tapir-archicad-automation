"""Manual test of SetElementNotificationClient's Archicad 26+ event handler options (issue #2267, part 5):
notifyOnAttributeChanges, notifyOnClassificationChanges, notifyOnPropertyDefinitionChanges (Archicad 26+),
notifyOnMarqueeChange (Archicad 27+) and notifyOn3DSettingsChanges (Archicad 30+).

Registers a client with every option the running Archicad has, waits 60 seconds and removes the client.
While it waits:
- create, rename and delete a layer in the Attribute Manager: expected attributesChanged with
  "Created", "Modified" and "Deleted";
- add a classification item in Classification Manager: expected classificationItemsChanged;
- add a custom property in Property Manager: expected propertyDefinitionsChanged
  (and propertyGroupsChanged for a new group);
- draw a marquee on the floor plan and remove it: expected marqueeChanged with type "rectangle"
  (or "polygon") and then "none";
- in the 3D window, change View > 3D View Options or Filter and Cut Elements in 3D: expected
  view3DSettingsChanged (navigating in 3D sends nothing).
After the client is removed, the same actions print nothing.
In Archicad 25 the command fails, and the options of later versions are left out in earlier ones.
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
import os, sys, threading, json, time

sys.path.insert (0, os.path.join (os.path.dirname (__file__), '..'))
import aclib  # noqa: E402

HOST = 'localhost'
PORT = 12352

class NotificationHandler (BaseHTTPRequestHandler):
    def do_POST (self):
        length = self.headers.get ('Content-Length')
        body = self.rfile.read (int (length)) if length else self.rfile.readline ()
        print (self.path, json.dumps (json.loads (body), indent = 4))
        self.send_response (200)
        self.end_headers ()

server = HTTPServer ((HOST, PORT), NotificationHandler)
threading.Thread (target = server.serve_forever, daemon = True).start ()

archicadVersion = aclib.RunCommand ('API.GetProductInfo', {})['version']

parameters = {
    'host': HOST,
    'port': PORT,
    'notifyOnNewElement': False,
    'notifyOnModificationOfAnElement': False,
    'notifyOnReservationChanges': False,
    'notifyOnAttributeChanges': True,
    'notifyOnClassificationChanges': True,
    'notifyOnPropertyDefinitionChanges': True
}
if archicadVersion >= 27:
    parameters['notifyOnMarqueeChange'] = True
if archicadVersion >= 30:
    parameters['notifyOn3DSettingsChanges'] = True

aclib.PrintJson (aclib.RunTapirCommand ('SetElementNotificationClient', parameters))

print ('Change attributes, classifications, properties, the marquee and the 3D settings now (60 seconds)...')
time.sleep (60)

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT}))
print ('Client removed; the same changes now should print nothing (5 seconds).')
time.sleep (5)
server.shutdown ()
