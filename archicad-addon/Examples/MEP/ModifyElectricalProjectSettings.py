"""Adds a wiring type and a load classification to the electrical project settings with ModifyElectricalProjectSettings.

Reads the settings back, then removes both again. Requires Archicad 30 or later.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('ModifyElectricalProjectSettings', {
    'wiringTypesToAdd': [{'name': 'Tapir Cu 2.5', 'crossSectionalArea': 2.5}],
    'loadClassificationsToAdd': [{'name': 'Tapir Kitchen', 'demandFactor': 0.7}]
}))
aclib.RunTapirCommand ('GetElectricalProjectSettings', {})

aclib.PrintJson (aclib.RunTapirCommand ('ModifyElectricalProjectSettings', {
    'wiringTypesToRemove': ['Tapir Cu 2.5'],
    'loadClassificationsToRemove': ['Tapir Kitchen']
}))
