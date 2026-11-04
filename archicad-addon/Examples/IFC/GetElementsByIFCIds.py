"""Finds elements by their IFC identifiers with GetElementsByIFCIds.

Needs at least one Wall: the IFC identifiers of the first walls are read with GetIFCIdsOfElements first.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]
if not walls:
    raise SystemExit ('No wall in the project.')

elementIFCIds = aclib.RunTapirCommand ('GetIFCIdsOfElements', {'elements': walls})['elementIFCIds']

aclib.PrintJson (aclib.RunTapirCommand ('GetElementsByIFCIds', {
    'ifcIds': [ids['ifcId'] for ids in elementIFCIds if 'ifcId' in ids]
}))
