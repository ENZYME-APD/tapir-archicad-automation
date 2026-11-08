"""Manual test of SetElementNotificationClient's notifyOnLockableReservationChanges (issue #2267, part 3).

Needs a Teamwork project. Registers a client that only wants reservation changes of lockable object
sets, waits 30 seconds and removes the client. While it waits, reserve and release the Composites
(Options > Element Attributes > Composites, Reserve, then Release in the Teamwork palette), and
Favorites. Ideally a second member reserves something too. Expected: one
{"lockableReservationChanged": {"event": "Reserved", "objectSetId": {"guid": ...},
"objectSetName": "Composites", "reservedBy": ..., "reservedByUserId": ...}} message per reservation
and one with "event": "Released" (no reservedBy) per release, and no element messages. After the
client is removed, reservations print nothing.
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
import os, sys, threading, json, time

sys.path.insert (0, os.path.join (os.path.dirname (__file__), '..'))
import aclib  # noqa: E402

HOST = 'localhost'
PORT = 12350

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
    'notifyOnLockableReservationChanges': True
}))

print ('Reserve and release the Composites and the Favorites now (30 seconds)...')
time.sleep (30)

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT}))
print ('Client removed; reservations now should print nothing (5 seconds).')
time.sleep (5)
server.shutdown ()
