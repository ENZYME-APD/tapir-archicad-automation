from http.server import BaseHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse
import aclib, threading, json

HOST = 'localhost'
PORT = 12345

class NotificationHandler(BaseHTTPRequestHandler):
    def do_POST(self):
        parsed_path = urlparse(self.path)
        if parsed_path.path == '/element_notification':
            post_data = self.rfile.read()
            notification = json.loads(post_data)
            if 'newElements' in notification:
                print('New elements:')
                self.dump_elements(notification['newElements'])
            if 'changedElements' in notification:
                print('Changed elements:')
                self.dump_elements(notification['changedElements'])
            if 'deletedElements' in notification:
                print('Deleted elements:')
                self.dump_elements(notification['deletedElements'])

    def dump_elements(self, elements):
        for element in elements:
            print(f"Element ID: {element['elementId']['guid']}, Type: {element['elementType']}")

def RunNotificationHandler(host, port):
    server = HTTPServer((host, port), NotificationHandler)
    server.serve_forever()

if __name__ == '__main__':
    threading.Thread(target=RunNotificationHandler, args=(HOST, PORT), daemon=True).start()
    aclib.RunTapirCommand ('SetElementNotificationClient', {'host': HOST, 'port': PORT},debug=False)
    input('Press Enter to stop...\n')