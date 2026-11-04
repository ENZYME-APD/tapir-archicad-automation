"""Prints whether Auto-Intersection (Options > Auto-Intersection) is on with GetAutoIntersect."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetAutoIntersect'))
