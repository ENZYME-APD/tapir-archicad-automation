"""Switches Bold Cut Lines of the current plan window with SetViewOptions, then restores the original options.

Run it with a floor plan, section or elevation window in front.
"""
import aclib

original = aclib.RunTapirCommand ('GetViewOptions')
boldCutLines = original['viewOptions']['onScreenViewOptions']['boldCutLines']

# Only the given option changes, the others keep their value.
aclib.PrintJson (aclib.RunTapirCommand ('SetViewOptions', {
    'viewOptions': {'onScreenViewOptions': {'boldCutLines': not boldCutLines}}
}))
aclib.RunTapirCommand ('GetViewOptions')

# Restore the original options and the original model view option combination.
aclib.PrintJson (aclib.RunTapirCommand ('SetViewOptions', {
    'modelViewOptionIndex': original['modelViewOptionIndex'],
    'viewOptions': original['viewOptions']
}))
