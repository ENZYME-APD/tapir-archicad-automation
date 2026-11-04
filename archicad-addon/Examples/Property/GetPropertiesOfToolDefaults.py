"""Lists the custom properties of the Wall and Slab tool defaults, with their values, with GetPropertiesOfToolDefaults."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertiesOfToolDefaults', {
    'elementTypes': ['Wall', 'Slab'],
    'filter': 'UserDefined',
    'includeValues': True
}))
