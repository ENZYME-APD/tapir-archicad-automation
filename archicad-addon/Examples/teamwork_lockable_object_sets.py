import aclib

# Non-element things Teamwork can reserve are addressed by the names the Archicad API documents
# for lockable object sets, e.g. Composites, LayerSettingsDialog, ProjectInfo, PreferencesDialog.
objectSetName = 'Composites'

aclib.RunTapirCommand (
    'GetLockableObjectSetStatus', {
        'objectSetName': objectSetName
    })

aclib.RunTapirCommand (
    'ReserveLockableObjectSet', {
        'objectSetName': objectSetName
    })

aclib.RunTapirCommand (
    'GetLockableObjectSetStatus', {
        'objectSetName': objectSetName
    })

aclib.RunTapirCommand (
    'ReleaseLockableObjectSet', {
        'objectSetName': objectSetName
    })

# The Hotlink and XRef management has its own reservation, separate from the named object sets.
aclib.RunTapirCommand ('ReserveHotlinkCacheManagement')

aclib.RunTapirCommand ('ReleaseHotlinkCacheManagement')
