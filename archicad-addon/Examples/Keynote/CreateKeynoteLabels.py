"""Creates a keynote item and places a Label showing its key and title with CreateKeynoteLabels.

Deletes the label and the keynote item at the end. Requires Archicad 28 or later.
"""
import aclib

item = aclib.RunTapirCommand ('CreateKeynoteItems', {
    'itemsData': [{'key': 'TAPIR-01', 'title': 'Tapir Example Keynote'}]
})['keynoteItemIdsOrErrors'][0]

labels = aclib.RunTapirCommand ('CreateKeynoteLabels', {
    'labelsData': [{
        'keynoteItemId': item['keynoteItemId'],
        'position': {'x': 0.0, 'y': 0.0},
        'contentFields': ['Key', 'Title']
    }]
})['elements']
aclib.PrintJson (labels)

aclib.RunTapirCommand ('DeleteElements', {'elements': [label for label in labels if 'elementId' in label]})
aclib.RunTapirCommand ('DeleteKeynoteItems', {'keynoteItemIds': [{'keynoteItemId': item['keynoteItemId']}]})
