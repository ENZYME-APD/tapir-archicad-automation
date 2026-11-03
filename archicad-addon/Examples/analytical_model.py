import aclib

# Requires Archicad 27 or newer (GetAnalyticalModelCheckTypeId requires Archicad 29 or newer).

currentAnalyticalModel = aclib.RunTapirCommand ('GetCurrentAnalyticalModel')

if currentAnalyticalModel and 'analyticalModelId' in currentAnalyticalModel:
    aclib.RunTapirCommand ('UpdateAnalyticalModel', {
            'analyticalModelId': currentAnalyticalModel['analyticalModelId']
        })

# Without an analyticalModelId the model of the current view is updated.
aclib.RunTapirCommand ('UpdateAnalyticalModel')

aclib.RunTapirCommand ('GetAnalyticalModelCheckTypeId')

aclib.RunTapirCommand ('SetAnalyticalGenerationSettingsToNoRule')
