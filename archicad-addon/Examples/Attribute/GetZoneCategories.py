"""Reads the code, color and zone stamp of every Zone Category with GetZoneCategories."""
import aclib

zoneCategories = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'ZoneCategory'}, debug = False)['attributes']

aclib.RunTapirCommand ('GetZoneCategories', {
    'attributeIds': [{'attributeId': zoneCategory['attributeId']} for zoneCategory in zoneCategories]
})
