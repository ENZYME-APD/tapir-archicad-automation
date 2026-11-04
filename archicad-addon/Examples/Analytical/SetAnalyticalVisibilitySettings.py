"""Toggles whether the nodes of the Structural Analytical Model are shown with SetAnalyticalVisibilitySettings.

Only the given fields change. Reads the setting back and restores it. Requires Archicad 26 or later.
"""
import aclib

settings = aclib.RunTapirCommand ('GetAnalyticalVisibilitySettings', {})['visibilitySettings']

aclib.PrintJson (aclib.RunTapirCommand ('SetAnalyticalVisibilitySettings', {
    'visibilitySettings': {'areNodesVisible': not settings['areNodesVisible']}
}))
aclib.RunTapirCommand ('GetAnalyticalVisibilitySettings', {})

aclib.PrintJson (aclib.RunTapirCommand ('SetAnalyticalVisibilitySettings', {
    'visibilitySettings': {'areNodesVisible': settings['areNodesVisible']}
}))
