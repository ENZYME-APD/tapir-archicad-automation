"""Gets the strings that property expressions use to refer to the Element ID and Width properties with GetPropertyExpressionReferences."""
import aclib

properties = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [
        {'type': 'BuiltIn', 'nonLocalizedName': 'General_ElementID'},
        {'type': 'BuiltIn', 'nonLocalizedName': 'General_Width'}
    ]
})['properties']

aclib.PrintJson (aclib.RunTapirCommand ('GetPropertyExpressionReferences', {
    'propertyIds': [p for p in properties if 'propertyId' in p]
}))
