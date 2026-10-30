import aclib

# Lists the dimension standards of the project (Options > Project Preferences > Dimensions).
# Available from Archicad 27; on older versions it returns an error.
response = aclib.RunTapirCommand ('GetDimensionStandards')

if response is not None and 'dimensionStandards' in response:
    for dimensionStandard in response['dimensionStandards']:
        linear = dimensionStandard['settings']['linear']
        print ('{}: {}, linear dimensions in {} with {} decimals'.format (
            dimensionStandard['index'], dimensionStandard['name'], linear['unit'], linear['decimals']))

    # Only the standard with the given name, e.g. the one a view uses (dimensionStyle of GetViewSettings).
    if response['dimensionStandards']:
        aclib.RunTapirCommand ('GetDimensionStandards', {
            'name': response['dimensionStandards'][0]['name']
        })
