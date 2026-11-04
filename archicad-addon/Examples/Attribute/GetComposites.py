"""Reads the usage, skins and separators of the first three Composites with GetComposites."""
import aclib

composites = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Composite'})['attributes'][:3]

aclib.PrintJson (aclib.RunTapirCommand ('GetComposites', {
    'attributeIds': [{'attributeId': composite['attributeId']} for composite in composites]
}))
