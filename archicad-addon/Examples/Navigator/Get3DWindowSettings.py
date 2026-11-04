"""Prints the pixel size and the zoom of the 3D window with Get3DWindowSettings."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('Get3DWindowSettings'))
