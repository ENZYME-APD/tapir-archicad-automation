"""Saves the open project to its file with SaveProject.

Overwrites the project file with the current state. An untitled project needs projectPath.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('SaveProject', {}))
