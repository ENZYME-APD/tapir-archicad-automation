"""Prints the version, release and build number of the running Archicad with GetArchicadReleaseInfo."""
import aclib

aclib.RunTapirCommand ('GetArchicadReleaseInfo')
