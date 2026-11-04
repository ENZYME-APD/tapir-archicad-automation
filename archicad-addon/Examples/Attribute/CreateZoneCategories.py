"""Creates a Zone Category with a code and a light blue color with CreateZoneCategories.

The zone stamp is copied from an existing Zone Category of the project. Overwrites the Zone Category of
the same name when the script is run again.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateZoneCategories', {
    'zoneCategoryDataArray': [{
        'name': 'Tapir Example Zone Category',
        'categoryCode': 'TAP',
        'color': {'red': 0.6, 'green': 0.8, 'blue': 1.0}
    }],
    'overwriteExisting': True
}))
