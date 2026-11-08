"""Manual test of SetElementNotificationClient's notifyOnViewEvents (issue #2267, part 2).

Registers a client that only wants Project Navigator item events, waits 30 seconds and removes the
client. While it waits, in the Navigator: save a view of the current window (View Map), rename it,
open it, then delete it; open a story in the Project Map and a layout in the Layout Book. Expected:
one {"viewEvent": {"event": ..., "navigatorMapId": ..., "navigatorItemId": {"guid": ...}}} message
per item event: "Inserted", "Modified", "Opened" and "Deleted" with navigatorMapId "PublicViewMap"
for the saved view, "Opened" with "ProjectMap" and "LayoutBook" for the story and the layout, and
no element messages. After the client is removed, navigator changes print nothing.
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
import os, sys, threading, json, time

sys.path.insert (0, os.path.join (os.path.dirname (__file__), '..'))
import aclib  # noqa: E402

HOST = 'localhost'
PORT = 12349

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
    'notifyOnViewEvents': True
}))

print ('Save, rename, open and delete a view, open a story and a layout now (30 seconds)...')
time.sleep (30)

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT}))
print ('Client removed; navigator changes now should print nothing (5 seconds).')
time.sleep (5)
server.shutdown ()
