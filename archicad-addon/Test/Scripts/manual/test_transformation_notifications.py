"""Manual test of the transformation in SetElementNotificationClient's changedElements (issue #2267, part 6).

Registers a client that observes the walls, waits 60 seconds and removes the client. While it
waits, select a wall in the floor plan and, with the Edit menu's commands:
- Move it (Drag) by 1 m to the right: expected a changedElements item with
  "transformation": {"type": "Drag", "displacement": {"x": 1.0, "y": 0.0, "z": 0.0}};
- Rotate it by 90 degrees: "type": "Rotate", "origin" the center of the rotation and "angle" about
  1.5708 (-1.5708 when rotated clockwise);
- Mirror it: "type": "Mirror", "origin" a point of the mirror axis and "axisDirection" its unit vector;
- Elevate it by 0.5 m: "type": "Elevate", "displacement": {"x": 0.0, "y": 0.0, "z": 0.5};
- Undo the last edit: a changedElements item without a transformation.
Changing the wall's settings in its settings dialog also gives an item without a transformation.
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
import os, sys, threading, json, time

sys.path.insert (0, os.path.join (os.path.dirname (__file__), '..'))
import aclib  # noqa: E402

HOST = 'localhost'
PORT = 12353

class NotificationHandler (BaseHTTPRequestHandler):
    def do_POST (self):
        length = self.headers.get ('Content-Length')
        body = self.rfile.read (int (length)) if length else self.rfile.readline ()
        print (self.path, json.dumps (json.loads (body), indent = 4))
        self.send_response (200)
        self.end_headers ()

server = HTTPServer ((HOST, PORT), NotificationHandler)
threading.Thread (target = server.serve_forever, daemon = True).start ()

aclib.PrintJson (aclib.RunTapirCommand ('SetElementNotificationClient', {
    'host': HOST,
    'port': PORT,
    'notifyOnNewElement': False,
    'notifyOnModificationOfAnElement': True,
    'notifyOnReservationChanges': False,
    'elementTypesToObserve': ['Wall']
}))

print ('Move, rotate, mirror and elevate a wall now, then undo (60 seconds)...')
time.sleep (60)

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT}))
server.shutdown ()
