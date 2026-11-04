"""Creates a copy of the first curve member's element from its analytical member with CreateElementsFromAnalyticalMembers.

Needs at least one analytical curve member; the member is passed on as GetAnalyticalMemberDetails returns it, so the
new beam or column lies on top of the original. Reading the members of the current view requires Archicad 27 or later.
"""
import aclib

curveMembers = aclib.RunTapirCommand ('GetAnalyticalMembers', {})['curveMembers']
if not curveMembers:
    raise SystemExit ('No analytical curve member in the current view.')
details = aclib.RunTapirCommand ('GetAnalyticalMemberDetails', {'elements': curveMembers[:1]})['analyticalMembers'][0]

aclib.PrintJson (aclib.RunTapirCommand ('CreateElementsFromAnalyticalMembers', {
    'members': [{'curveMember': details['curveMember']}]
}))
