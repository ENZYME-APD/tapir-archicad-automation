"""Manual test of SetElementNotificationClient's notifyOnToolChange (issue #2267, part 1).

Registers a client that only wants tool changes, waits 20 seconds and removes the client. While it
waits, pick a few tools in Archicad's Toolbox (Wall, Column, Slab, Object, then the Arrow and the
Marquee). Expected: one {"toolChanged": {"elementType": ...}} message per switch, with the element
type of the picked tool ("Wall", "Column", ...) and "None" for the Arrow and the Marquee, and no
element or selection messages. After the client is removed, switching tools prints nothing.
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
import os, sys, threading, json, time

sys.path.insert (0, os.path.join (os.path.dirname (__file__), '..'))
import aclib  # noqa: E402

HOST = 'localhost'
PORT = 12348

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
    'notifyOnModificationOfAnElement': False,
    'notifyOnReservationChanges': False,
    'notifyOnToolChange': True
}))

print ('Switch tools in the Toolbox now (20 seconds)...')
time.sleep (20)

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT}))
print ('Client removed; switching tools now should print nothing (5 seconds).')
time.sleep (5)
server.shutdown ()
