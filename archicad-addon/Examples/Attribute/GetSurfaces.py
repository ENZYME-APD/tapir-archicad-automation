"""Reads the material type, colors and texture of the first three Surfaces with GetSurfaces."""
import aclib

surfaces = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Surface'}, debug = False)['attributes'][:3]

aclib.RunTapirCommand ('GetSurfaces', {
    'attributeIds': [{'attributeId': surface['attributeId']} for surface in surfaces],
    'fields': ['materialType', 'surfaceColor', 'transparency', 'texture']
})
