"""Prints the element events Archicad sends to a local HTTP server registered with SetElementNotificationClient.

Creates a column to trigger a notification, then removes the client again. Archicad posts each event
as JSON to http://<host>:<port>/element_notification.

Modifications are observed only for columns here (elementTypesToObserve): attaching an observer to
every element blocks Archicad for minutes on a project with thousands of elements.
"""
import aclib
import json
import threading
import time
from http.server import BaseHTTPRequestHandler, HTTPServer

HOST = 'localhost'
PORT = 12345

class NotificationHandler (BaseHTTPRequestHandler):
    def do_POST (self):
        length = self.headers.get ('Content-Length')
        body = self.rfile.read (int (length)) if length else self.rfile.readline ()
        print (json.dumps (json.loads (body), indent = 4))
        self.send_response (200)
        self.end_headers ()

server = HTTPServer ((HOST, PORT), NotificationHandler)
threading.Thread (target = server.serve_forever, daemon = True).start ()

aclib.PrintJson (aclib.RunTapirCommand ('SetElementNotificationClient', {
    'host': HOST,
    'port': PORT,
    'notifyOnNewElement': True,
    'notifyOnModificationOfAnElement': True,
    'elementTypesToObserve': ['Column'],
    'notifyOnReservationChanges': False
}))

aclib.RunTapirCommand ('CreateColumns', {'columnsData': [{'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0}}]})
time.sleep (2)

aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT})
server.shutdown ()
