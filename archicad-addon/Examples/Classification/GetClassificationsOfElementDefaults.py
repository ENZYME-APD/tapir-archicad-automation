"""Reads the classification of the Wall and Slab tool defaults in every classification system with GetClassificationsOfElementDefaults."""
import aclib

systems = aclib.RunTapirCommand ('GetAllClassificationSystems')['classificationSystems']

aclib.PrintJson (aclib.RunTapirCommand ('GetClassificationsOfElementDefaults', {
    'elementTypes': ['Wall', 'Slab'],
    'classificationSystemIds': [{'classificationSystemId': system['classificationSystemId']} for system in systems]
}))
