"""Creates a Layout Book subset with its own ID prefix and numbering, using CreateLayoutSubset.

The new subset stays in the Layout Book: subsets cannot be deleted through the Archicad API.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateLayoutSubset', {
    'subsetsData': [{
        'name': 'Tapir Example Subset',
        'addOwnPrefix': True,
        'ownPrefix': 'TX',
        'numberingStyle': '01',
        'startAt': 1
    }]
}))
