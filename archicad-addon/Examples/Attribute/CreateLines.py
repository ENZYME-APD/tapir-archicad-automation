"""Creates a dashed Line type (3 mm dash, 1.5 mm gap on the printout) with CreateLines.

Overwrites the line type of the same name when the script is run again.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('CreateLines', {
    'lineDataArray': [{
        'name': 'Tapir Example Dashed Line',
        'scaleWithPlan': False,
        'lineType': 'Dashed',
        'period': 4.5,
        'dashItems': [{'dash': 3.0, 'gap': 1.5}]
    }],
    'overwriteExisting': True
}))
