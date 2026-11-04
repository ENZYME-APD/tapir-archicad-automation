"""Reads the usage, size and skins of the first three Profiles with GetProfiles."""
import aclib

profiles = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'Profile'}, debug = False)['attributes'][:3]

aclib.RunTapirCommand ('GetProfiles', {
    'attributeIds': [{'attributeId': profile['attributeId']} for profile in profiles],
    'fields': ['useWith', 'width', 'height', 'skins']
})
