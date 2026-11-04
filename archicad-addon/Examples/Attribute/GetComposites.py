"""Reads the usage, skins and separators of the first three Composites with GetComposites."""
import aclib

composites = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Composite'}, debug = False)['attributes'][:3]

aclib.RunTapirCommand ('GetComposites', {
    'attributeIds': [{'attributeId': composite['attributeId']} for composite in composites]
})
