"""Creates a keynote item and prints the autotext tokens that reference its fields, with GetKeynoteAutoTexts.

Deletes the keynote item at the end. Requires Archicad 28 or later.
"""
import aclib

item = aclib.RunTapirCommand ('CreateKeynoteItems', {
    'itemsData': [{'key': 'TAPIR-01', 'title': 'Tapir Example Keynote'}]
})['keynoteItemIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('GetKeynoteAutoTexts', {'keynoteItems': [{'keynoteItemId': item['keynoteItemId']}]}))

aclib.RunTapirCommand ('DeleteKeynoteItems', {'keynoteItemIds': [{'keynoteItemId': item['keynoteItemId']}]})
