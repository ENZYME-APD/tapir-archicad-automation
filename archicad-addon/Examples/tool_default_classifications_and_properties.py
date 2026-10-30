import aclib

# Issue #1527 part 3: every classification and the properties of a tool's defaults (the
# settings new elements get), read with GetClassificationsOfToolDefaults and
# GetPropertiesOfToolDefaults. Unlike GetClassificationsOfElementDefaults, the first one
# needs no list of classification systems.

response = aclib.RunTapirCommand ('GetClassificationsOfToolDefaults', {
    'elementTypes': ['Wall', 'Slab', 'NotAnElementType']})

response = aclib.RunTapirCommand ('GetPropertiesOfToolDefaults', {
    'elementTypes': ['Wall'],
    'filter': 'UserDefined',
    'includeValues': True})

# Only the definitions of every property, without the values.
response = aclib.RunTapirCommand ('GetPropertiesOfToolDefaults', {
    'elementTypes': ['Column']}, debug = False)
properties = response['propertiesOfToolDefaults'][0].get ('properties', [])
print ('The Column tool defaults have {} properties.'.format (len (properties)))
