"""Creates a keynote item and changes its title and description with ModifyKeynoteItems.

Deletes the item at the end. Requires Archicad 28 or later.
"""
import aclib

item = aclib.RunTapirCommand ('CreateKeynoteItems', {
    'itemsData': [{'key': 'TAPIR-01', 'title': 'Tapir Example Keynote'}]
})['keynoteItemIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('ModifyKeynoteItems', {
    'itemsData': [{
        'keynoteItemId': item['keynoteItemId'],
        'title': 'Tapir Renamed Keynote',
        'description': 'Changed by the Tapir example.'
    }]
}))

aclib.RunTapirCommand ('DeleteKeynoteItems', {'keynoteItemIds': [{'keynoteItemId': item['keynoteItemId']}]})
