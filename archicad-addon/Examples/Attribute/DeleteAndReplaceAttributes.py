"""Creates two temporary Line types and deletes the first with DeleteAndReplaceAttributes, replaced by the second, first as a dry run.

The script deletes the second line type at the end. Requires Archicad 27 or later.
"""
import aclib

lineIds = aclib.RunTapirCommand ('CreateLines', {'lineDataArray': [
    {'name': 'Tapir Example Line To Delete'},
    {'name': 'Tapir Example Replacement Line'}
]})['attributeIds']
for lineId in lineIds:
    if 'error' in lineId:
        raise SystemExit ('Could not create the line type: ' + lineId['error']['message'])
toDelete, replacement = lineIds

parameters = {
    'attributeType': 'Line',
    'attributes': [{'attributeId': toDelete['attributeId'], 'replacementAttributeId': replacement['attributeId']}]
}
aclib.PrintJson (aclib.RunTapirCommand ('DeleteAndReplaceAttributes', dict (parameters, dryRun = True)))
aclib.PrintJson (aclib.RunTapirCommand ('DeleteAndReplaceAttributes', parameters))

aclib.RunTapirCommand ('DeleteAttributes', {
    'attributesToDelete': [{'attributeType': 'Line', 'attributeId': replacement}]
})
