"""Tells with GetTeamworkProjectInfo whether the open project is a Teamwork project, and its server, project name and current user."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetTeamworkProjectInfo'))
