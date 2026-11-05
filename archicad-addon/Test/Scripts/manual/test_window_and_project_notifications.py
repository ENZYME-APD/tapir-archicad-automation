"""Manual test of SetElementNotificationClient's notifyOnWindowChange and notifyOnProjectEvents (issue #1813).

Registers a client that only wants window and project events, switches to the 3D window, back to the
floor plan and to another story with ChangeWindow, then removes the client. Expected: windowChanged
messages ("event": "Window" or "Floor") with the new windowType, its databaseId and the active
storyIndex, and no element or selection messages. While it waits you can also switch windows or
stories by hand, or Send & Receive in a Teamwork project: each event should be printed
("projectEvent": "SendChanges" / "ReceiveChanges").
"""
from http.server import BaseHTTPRequestHandler, HTTPServer
import os, sys, threading, json, time

sys.path.insert (0, os.path.join (os.path.dirname (__file__), '..'))
import aclib  # noqa: E402

HOST = 'localhost'
PORT = 12347

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
    'notifyOnWindowChange': True,
    'notifyOnProjectEvents': True
}))

aclib.RunTapirCommand ('ChangeWindow', {'windowType': '3DModel'})
time.sleep (1)
aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan'})
time.sleep (1)
stories = aclib.RunTapirCommand ('GetStories')
if stories['lastStory'] > stories['firstStory']:
    otherStory = stories['actStory'] + 1 if stories['actStory'] < stories['lastStory'] else stories['actStory'] - 1
    aclib.RunTapirCommand ('ChangeWindow', {'windowType': 'FloorPlan', 'storyIndex': otherStory})
time.sleep (10)

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': HOST, 'port': PORT}))
server.shutdown ()
