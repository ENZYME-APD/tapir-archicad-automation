"""Prints the settings of the first named model view option combination with GetModelViewOptionDetails, then lists the library part options of every combination.

Requires Archicad 27 or later.
"""
import aclib

# Archicad may list a combination without a name once the view options of a window changed.
names = [modelViewOption['name'] for modelViewOption in aclib.RunTapirCommand ('GetModelViewOptions')['modelViewOptions'] if modelViewOption['name']]
if not names:
    raise SystemExit ('No model view option combination in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetModelViewOptionDetails', {
    'modelViewOptions': [{'name': names[0]}]
}))

for details in aclib.RunTapirCommand ('GetModelViewOptionDetails')['modelViewOptionDetails']:
    if 'error' in details:
        continue
    print (details['name'], [gdlOption['name'] for gdlOption in details['gdlOptions']])
