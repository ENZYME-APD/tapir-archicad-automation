"""Reads which Pen Tables are active for the model and for the layouts with GetPenTables.

The pens field is left out, as it lists 255 pens per table.
"""
import aclib

penTables = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'PenTable'})['attributes']

aclib.PrintJson (aclib.RunTapirCommand ('GetPenTables', {
    'attributeIds': [{'attributeId': penTable['attributeId']} for penTable in penTables],
    'fields': ['isActiveForModel', 'isActiveForLayout']
}))
