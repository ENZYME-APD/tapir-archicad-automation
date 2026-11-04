"""Receives the changes of the other team members from the BIMcloud server with TeamworkReceive.

Needs an open Teamwork project.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('TeamworkReceive'))
