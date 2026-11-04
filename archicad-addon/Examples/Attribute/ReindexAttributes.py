"""Creates a temporary Line type and moves it to the next free index with ReindexAttributes, first as a dry run.

The line type gets a new attribute id at its new index; the script deletes it at the end. Requires Archicad 27 or later.
"""
import aclib

name = 'Tapir Example Line To Reindex'
lineId = aclib.RunTapirCommand ('CreateLines', {'lineDataArray': [{'name': name}]})['attributeIds'][0]
if 'error' in lineId:
    raise SystemExit ('Could not create the line type: ' + lineId['error']['message'])

lines = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Line'})['attributes']
newIndex = int (max (line['index'] for line in lines)) + 1

parameters = {
    'attributeType': 'Line',
    'attributes': [{'attributeId': lineId['attributeId'], 'newIndex': newIndex}]
}
aclib.PrintJson (aclib.RunTapirCommand ('ReindexAttributes', dict (parameters, dryRun = True)))
aclib.PrintJson (aclib.RunTapirCommand ('ReindexAttributes', parameters))

lines = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Line'})['attributes']
aclib.RunTapirCommand ('DeleteAttributes', {
    'attributesToDelete': [{'attributeType': 'Line', 'attributeId': {'attributeId': line['attributeId']}} for line in lines if line['name'] == name]
})
