"""Switches the fill handles of the current plan window with SetViewOptions, then restores the original options.

Run it with a floor plan, section or elevation window in front.
"""
import aclib

original = aclib.RunTapirCommand ('GetViewOptions')
showFillHandles = original['viewOptions']['onScreenViewOptions']['showFillHandles']

# Only the given option changes, the others keep their value.
aclib.PrintJson (aclib.RunTapirCommand ('SetViewOptions', {
    'viewOptions': {'onScreenViewOptions': {'showFillHandles': not showFillHandles}}
}))
aclib.RunTapirCommand ('GetViewOptions')

# Restore the original options. Archicad may renumber the model view option combinations when a
# window is switched to one, so the original combination is looked up by its name.
if original['modelViewOptionName']:
    combination = aclib.RunTapirCommand ('FindModelViewOption', {'name': original['modelViewOptionName']})
    aclib.PrintJson (aclib.RunTapirCommand ('SetViewOptions', {
        'modelViewOptionIndex': combination['index'],
        'viewOptions': {'onScreenViewOptions': original['viewOptions']['onScreenViewOptions']}
    }))
else:
    aclib.PrintJson (aclib.RunTapirCommand ('SetViewOptions', {
        'viewOptions': original['viewOptions']
    }))
