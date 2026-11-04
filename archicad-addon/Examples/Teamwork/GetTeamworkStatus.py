"""Gets with GetTeamworkStatus whether the project is shared and online, its members and who holds the Hotlink and XRef management.

In a solo project isShared and isOnline are false.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetTeamworkStatus'))
