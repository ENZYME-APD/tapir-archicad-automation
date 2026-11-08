"""Lets the user pick a layer with PickAttribute and waits for the user (Archicad 26 and later).

A small dialog shows Archicad's attribute picker; the response is the chosen attribute, or cancelled. The
attributeType can also be BuildingMaterial, Composite, Profile or Surface, and Fill or Line from Archicad 27.
"""
import aclib

result = aclib.RunTapirCommand ('PickAttribute', {'attributeType': 'Layer', 'title': 'Choose a layer'})
if result is not None:
    if result['cancelled']:
        print ('Cancelled.')
    else:
        print ('Chosen {}: {}'.format (result['attributeType'], result['attributeId']['guid']))
