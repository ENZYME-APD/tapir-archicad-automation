"""Creates a keynote item and deletes it with DeleteKeynoteItems.

Requires Archicad 28 or later.
"""
import aclib

item = aclib.RunTapirCommand ('CreateKeynoteItems', {
    'itemsData': [{'key': 'TAPIR-01', 'title': 'Tapir Example Keynote'}]
})['keynoteItemIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('DeleteKeynoteItems', {'keynoteItemIds': [{'keynoteItemId': item['keynoteItemId']}]}))
