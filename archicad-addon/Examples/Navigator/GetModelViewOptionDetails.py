"""Prints the settings of the first model view option combination with GetModelViewOptionDetails, then lists the library part options of every combination.

Requires Archicad 27 or later.
"""
import aclib

modelViewOptions = aclib.RunTapirCommand ('GetModelViewOptions')['modelViewOptions']
if not modelViewOptions:
    raise SystemExit ('No model view option combination in the project.')

aclib.PrintJson (aclib.RunTapirCommand ('GetModelViewOptionDetails', {
    'modelViewOptions': [{'name': modelViewOptions[0]['name']}]
}))

for details in aclib.RunTapirCommand ('GetModelViewOptionDetails')['modelViewOptionDetails']:
    if 'error' in details:
        continue
    print (details['name'], [gdlOption['name'] for gdlOption in details['gdlOptions']])
