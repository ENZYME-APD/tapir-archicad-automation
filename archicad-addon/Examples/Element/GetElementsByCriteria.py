"""Finds walls by story and by a property value with GetElementsByCriteria, first counting them only.

The property criterion compares the wall thickness in metres, whatever the project's display units.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetElementsByCriteria', {'types': ['Wall'], 'countOnly': True}))

aclib.PrintJson (aclib.RunTapirCommand ('GetElementsByCriteria', {
    'types': ['Wall'],
    'floorIndices': [0],
    'limit': 5
}))

thickness = aclib.RunCommand ('API.GetPropertyIds', {
    'properties': [{'type': 'BuiltIn', 'nonLocalizedName': 'General_Thickness'}]
})['properties'][0]['propertyId']

aclib.PrintJson (aclib.RunTapirCommand ('GetElementsByCriteria', {
    'types': ['Wall'],
    'propertyCriteria': [{'propertyId': thickness, 'operator': 'greaterThan', 'value': '0.2'}],
    'limit': 5
}))
