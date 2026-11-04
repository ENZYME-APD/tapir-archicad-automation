"""Turns Auto-Intersection off with SetAutoIntersect, reads it back, then restores the previous state.

It is a setting of the running Archicad, not of the project.
"""
import aclib

originalIsOn = aclib.RunTapirCommand ('GetAutoIntersect')['isOn']

aclib.RunTapirCommand ('SetAutoIntersect', {'isOn': False})
aclib.RunTapirCommand ('GetAutoIntersect')

aclib.RunTapirCommand ('SetAutoIntersect', {'isOn': originalIsOn})
