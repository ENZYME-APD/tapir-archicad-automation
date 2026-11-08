"""Prints the license type, the license flags and the SSA level of the running Archicad with GetArchicadLicenseInfo,
then warns when the license is not a full one.

The SSA level is not reported in Archicad 25.
"""
import aclib

licenseInfo = aclib.RunTapirCommand ('GetArchicadLicenseInfo')
aclib.PrintJson (licenseInfo)

if licenseInfo and licenseInfo['licenseType'] != 'Full':
    print (f"Warning: Archicad runs on a {licenseInfo['licenseType']} license.")
