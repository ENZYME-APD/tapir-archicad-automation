"""Reads the type, usage and spacing of the first three Fills with GetFills."""
import aclib

fills = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Fill'}, debug = False)['attributes'][:3]

aclib.RunTapirCommand ('GetFills', {
    'attributeIds': [{'attributeId': fill['attributeId']} for fill in fills],
    'fields': ['subType', 'useForWalls', 'useForDraft', 'useForCover', 'horizontalSpacing', 'verticalSpacing', 'angle']
})
