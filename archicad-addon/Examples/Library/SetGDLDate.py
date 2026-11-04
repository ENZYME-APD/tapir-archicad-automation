"""Sets the date GDL scripts see with SetGDLDate.

Sets the value read with GetGDLDate, so the date stays unchanged; give another value in the same format to test
date-dependent library parts, then set the original back.
"""
import aclib

originalDate = aclib.RunTapirCommand ('GetGDLDate')['date']

aclib.PrintJson (aclib.RunTapirCommand ('SetGDLDate', {'date': originalDate}))

aclib.RunTapirCommand ('GetGDLDate')
