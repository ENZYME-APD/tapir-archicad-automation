"""Flips a Boolean parameter of the cut symbol of the first opening with SetOpeningSymbolParameters, then restores it.

Needs an Opening (made with the Opening tool) with a cut symbol that has a Boolean parameter.
Requires Archicad 29 or later.
"""
import aclib

openings = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Opening'})['elements'][:1]
if not openings:
    raise SystemExit ('No opening in the project.')

symbol = aclib.RunTapirCommand ('GetOpeningSymbolParameters', {'elements': openings})['openings'][0]
booleans = [p for p in symbol.get ('cutSymbolParameters', []) if p['type'] == 'Boolean' and 'dimension1' not in p]
if not booleans:
    raise SystemExit ('The first opening has no cut symbol with a Boolean parameter.')

parameter = booleans[0]
aclib.PrintJson (aclib.RunTapirCommand ('SetOpeningSymbolParameters', {'elements': [{
    'elementId': openings[0]['elementId'],
    'cutSymbolParameters': [{'name': parameter['name'], 'value': not parameter['value']}]
}]}))
aclib.RunTapirCommand ('GetOpeningSymbolParameters', {'elements': openings})

aclib.PrintJson (aclib.RunTapirCommand ('SetOpeningSymbolParameters', {'elements': [{
    'elementId': openings[0]['elementId'],
    'cutSymbolParameters': [{'name': parameter['name'], 'value': parameter['value']}]
}]}))
