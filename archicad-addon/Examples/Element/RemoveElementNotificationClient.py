"""Registers an element notification client and removes it again with RemoveElementNotificationClient.

host and port name the client given to SetElementNotificationClient.
"""
import aclib

aclib.RunTapirCommand ('SetElementNotificationClient', {'host': 'localhost', 'port': 12345})

aclib.PrintJson (aclib.RunTapirCommand ('RemoveElementNotificationClient', {'host': 'localhost', 'port': 12345}))
