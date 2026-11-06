from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse
import aclib, threading, json, time

HOST = 'localhost'
PORT = 12345

class NotificationHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        parsed_path = urlparse(self.path)
        if parsed_path.path == '/element_notification':
            post_data = self.rfile.read()
            notification = json.loads(post_data)
            print(json.dumps(notification, indent=3))
            if 'newElements' in notification:
                for newElement in notification['newElements']:
                    continue
            if 'changedElements' in notification:
                for changedElement in notification['changedElements']:
                    continue
            if 'deletedElements' in notification:
                for deletedElement in notification['deletedElements']:
                    continue
            if 'reservedElements' in notification:
                for reservedElement in notification['reservedElements']:
                    continue
            if 'releasedElements' in notification:
                for releasedElement in notification['releasedElements']:
                    continue

def RunNotificationHandler(host, port):
    server = HTTPServer((host, port), NotificationHandler)
    server.serve_forever()

if __name__ == '__main__':
    threading.Thread(target=RunNotificationHandler, args=(HOST, PORT), daemon=True).start()
    aclib.RunTapirCommand ('SetElementNotificationClient', {'host': HOST, 'port': PORT})

    # Create some elements to trigger notifications
    aclib.RunTapirCommand (
        'CreateColumns', {
            'columnsData': [
                {'coordinates': {'x': 0.0, 'y': 0.0, 'z': 0.0}},
                {'coordinates': {'x': 1.0, 'y': 1.0, 'z': 1.0}}
            ]
        })
    time.sleep(1)