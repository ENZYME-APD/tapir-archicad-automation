"""Reads the type, period and dashes of the first three Line types with GetLines."""
import aclib

lines = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Line'}, debug = False)['attributes'][:3]

aclib.RunTapirCommand ('GetLines', {
    'attributeIds': [{'attributeId': line['attributeId']} for line in lines],
    'fields': ['lineType', 'scaleWithPlan', 'period', 'dashItems']
})
