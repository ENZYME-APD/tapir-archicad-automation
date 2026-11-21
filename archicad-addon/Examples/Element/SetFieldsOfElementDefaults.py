"""Gives the Wall tool defaults pen 5 for the contour with SetFieldsOfElementDefaults, then puts the old pen back.

The walls created in between get pen 5.
"""
import aclib

old = aclib.RunTapirCommand ('GetFieldsOfElementDefaults', {'elementTypes': ['Wall'], 'fields': ['contPen']})
oldPen = old['fieldsOfElementDefaults'][0]['fields']['contPen']

aclib.PrintJson (aclib.RunTapirCommand ('SetFieldsOfElementDefaults', {
    'fieldsOfElementDefaults': [{'elementType': 'Wall', 'fields': {'contPen': 5}}]
}))

aclib.RunTapirCommand ('SetFieldsOfElementDefaults', {
    'fieldsOfElementDefaults': [{'elementType': 'Wall', 'fields': {'contPen': oldPen}}]
})
