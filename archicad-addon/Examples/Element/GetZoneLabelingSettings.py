"""Reads whether zones are labeled automatically and which Label Favorite is placed, with GetZoneLabelingSettings.

Requires Archicad 28 or later.
"""
import aclib

aclib.RunTapirCommand ('GetZoneLabelingSettings', {})
