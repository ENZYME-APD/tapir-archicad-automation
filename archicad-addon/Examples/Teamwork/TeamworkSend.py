"""Sends your changes to the BIMcloud server with TeamworkSend.

Needs an open Teamwork project.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('TeamworkSend'))
