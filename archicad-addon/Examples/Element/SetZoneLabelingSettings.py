"""Switches the automatic labeling of zones on or off with SetZoneLabelingSettings, reads it back, then restores it.

Requires Archicad 28 or later.
"""
import aclib

original = aclib.RunTapirCommand ('GetZoneLabelingSettings', {})

aclib.PrintJson (aclib.RunTapirCommand ('SetZoneLabelingSettings', {'automaticLabeling': not original['isAutomaticLabelingOn']}))
aclib.RunTapirCommand ('GetZoneLabelingSettings', {})

aclib.RunTapirCommand ('SetZoneLabelingSettings', {'automaticLabeling': original['isAutomaticLabelingOn']})
