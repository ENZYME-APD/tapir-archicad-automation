"""Stores a key-value pair on the first Layer with SetUserDataOfAttributes, reads it back, then removes the key.

Undo does not revert user data changes.
"""
import aclib

layer = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Layer'}, debug = False)['attributes'][0]
attributeIds = [{'attributeId': layer['attributeId']}]

aclib.RunTapirCommand ('SetUserDataOfAttributes', {
    'attributeIds': attributeIds,
    'pairs': [{'key': 'tapirExample.note', 'value': 'Checked by a Tapir example'}]
})
aclib.RunTapirCommand ('GetUserDataOfAttributes', {'attributeIds': attributeIds})

aclib.RunTapirCommand ('SetUserDataOfAttributes', {
    'attributeIds': attributeIds,
    'keysToDelete': ['tapirExample.note']
})
