"""Prints whether the 3D cut is on and the cut planes of the 3D window with Get3DCutPlanes."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('Get3DCutPlanes'))
