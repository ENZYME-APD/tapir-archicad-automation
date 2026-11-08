"""Creates a Pen Table as a copy of the current one, with a red pen 1, using CreatePenTables.

The new Pen Table is not activated. Overwrites the Pen Table of the same name when the script is run again.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreatePenTables', {
    'penTableDataArray': [{
        'name': 'Tapir Example Pen Table',
        'isActiveForModel': False,
        'isActiveForLayout': False,
        'pens': [{'index': 1, 'color': {'red': 1.0, 'green': 0.0, 'blue': 0.0}, 'width': 0.35, 'description': 'Red'}]
    }],
    'overwriteExisting': True
}))
