"""Manual test of SetElementNotificationClient's notifyOnSelectionChange (issue #1813).

Registers a client that only wants selection changes, selects two elements, clears the selection
and removes the client. Expected: at least one {"selectedElements": [...]} message listing the
selected elements, a final one with an empty list, and no newElements/changedElements messages.
While it waits you can also click elements in Archicad: each selection change should be printed.
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
import os, sys, threading, json, time

sys.path.insert (0, os.path.join (os.path.dirname (__file__), '..'))
import aclib  # noqa: E402

HOST = 'localhost'
PORT = 12346

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
    'notifyOnSelectionChange': True
}))

elements = aclib.RunCommand ('API.GetAllElements', {})['elements'][:2]
aclib.RunTapirCommand ('ChangeSelectionOfElements', {'addElementsToSelection': elements})
time.sleep (1)
aclib.RunTapirCommand ('ChangeSelectionOfElements', {'removeElementsFromSelection': elements})
time.sleep (10)

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT}))
server.shutdown ()
