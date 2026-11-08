"""Prints the names of the rendering scenes and the current scene with GetRenderingScenes."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetRenderingScenes'))
