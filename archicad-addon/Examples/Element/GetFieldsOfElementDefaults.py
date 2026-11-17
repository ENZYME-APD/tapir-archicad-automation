"""Reads the thickness, height and pen of the Wall and Slab tool defaults with GetFieldsOfElementDefaults."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetFieldsOfElementDefaults', {
    'elementTypes': ['Wall', 'Slab'],
    'fields': ['thickness', 'height', 'contPen']
}))
