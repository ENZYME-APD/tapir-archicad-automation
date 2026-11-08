"""Manual test of SetElementNotificationClient's notifyOnToolDefaultChanges (issue #2267, part 4).

Registers a client that only wants tool default changes, waits 30 seconds and removes the client.
While it waits, deselect everything, open the Wall tool's settings dialog, change the thickness and
press OK; then do the same with the Column tool. Expected: one
{"toolDefaultsChanged": {"elementType": "Wall"}} message for the wall defaults and one with
"Column" for the column defaults, and no element messages. After the client is removed, changing
the defaults prints nothing.
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
import os, sys, threading, json, time

sys.path.insert (0, os.path.join (os.path.dirname (__file__), '..'))
import aclib  # noqa: E402

HOST = 'localhost'
PORT = 12351

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
    'notifyOnToolDefaultChanges': True
}))

print ('Change the default settings of the Wall and the Column tool now (30 seconds)...')
time.sleep (30)

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT}))
print ('Client removed; changing the defaults now should print nothing (5 seconds).')
time.sleep (5)
server.shutdown ()
