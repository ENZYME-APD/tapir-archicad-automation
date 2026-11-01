import aclib

originalSettings = aclib.RunTapirCommand ('GetZoneLabelingSettings', {})

aclib.RunTapirCommand (
    'SetZoneLabelingSettings', {
        'automaticLabeling': False
    })

aclib.RunTapirCommand (
    'SetZoneLabelingSettings', {
        'automaticLabeling': True
    })

aclib.RunTapirCommand ('GetZoneLabelingSettings', {})

# The Favorite must be a Label Favorite of the project
if 'selectedFavoriteName' in originalSettings:
    aclib.RunTapirCommand (
        'SetZoneLabelingSettings', {
            'favoriteName': originalSettings['selectedFavoriteName']
        })

if 'isAutomaticLabelingOn' in originalSettings:
    aclib.RunTapirCommand (
        'SetZoneLabelingSettings', {
            'automaticLabeling': originalSettings['isAutomaticLabelingOn']
        })
