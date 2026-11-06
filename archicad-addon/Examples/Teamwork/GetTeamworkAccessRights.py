"""Gets with GetTeamworkAccessRights whether the role of the current user has a few Teamwork access rights.

Without accessRights every right of the Archicad version is returned.
"""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetTeamworkAccessRights', {
    'accessRights': ['LayersCreate', 'LibraryPartDeleteModify', 'HotlinkAndXrefManage']
}))
