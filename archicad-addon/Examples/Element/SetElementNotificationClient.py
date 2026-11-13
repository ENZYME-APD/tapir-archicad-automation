"""Prints the element events Archicad sends to a local HTTP server registered with SetElementNotificationClient.

Creates a column to trigger a notification, moves it to trigger a change notification, then removes
the client again. Archicad posts each event as JSON to http://<host>:<port>/element_notification.

Modifications are observed only for the columns (elementTypesToObserve) and for the listed elements
(elementsToObserve, here the first wall of the project, if there is one): attaching an observer to
every element blocks Archicad for minutes on a project with thousands of elements, while a list of
elements takes time only for the listed ones.
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

walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']

parameters = {
    'host': HOST,
    'port': PORT,
    'notifyOnNewElement': True,
    'notifyOnModificationOfAnElement': True,
    'elementTypesToObserve': ['Column'],
    'notifyOnReservationChanges': False
}
if walls:
    parameters['elementsToObserve'] = walls[:1]
aclib.PrintJson (aclib.RunTapirCommand ('SetElementNotificationClient', parameters))

columns = aclib.RunTapirCommand ('CreateColumns', {'columnsData': [{'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0}}]})['elements']
aclib.RunTapirCommand ('MoveElements', {'elementsWithMoveVectors': [
    {'elementId': columns[0]['elementId'], 'moveVector': {'x': 1.0, 'y': 0.0, 'z': 0.0}}]})
time.sleep (2)

aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT})
server.shutdown ()
