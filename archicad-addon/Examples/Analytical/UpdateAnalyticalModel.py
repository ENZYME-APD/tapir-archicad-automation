"""Updates the Structural Analytical Model of the current view with UpdateAnalyticalModel.

Leave out 'analyticalModelId' to update the model of the current view. Requires Archicad 27 or later.
"""
import aclib

analyticalModelId = aclib.RunTapirCommand ('GetCurrentAnalyticalModel', {})['analyticalModelId']

aclib.PrintJson (aclib.RunTapirCommand ('UpdateAnalyticalModel', {
    'analyticalModelId': analyticalModelId
}))
