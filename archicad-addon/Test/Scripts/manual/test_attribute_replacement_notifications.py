"""Manual test of SetElementNotificationClient's notifyOnAttributeReplacement (issue #1813).

Registers a client that only wants attribute replacement events and waits 60 seconds. Meanwhile, in
Archicad, delete an attribute and replace it with another one (Options > Element Attributes >
Attribute Manager, or e.g. Line Types / Fills: Delete and Replace). Expected: a replacedAttributes
message with the attributeType, the oldIndex and newIndex and the newAttributeId of the replacing
attribute, and no element messages. Then the client is removed. In Archicad 25 the
SetElementNotificationClient call fails instead (not available there).
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
    'notifyOnAttributeReplacement': True
}))

print ('Delete and replace an attribute in Archicad within 60 seconds...')
time.sleep (60)

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT}))
server.shutdown ()
