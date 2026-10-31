import aclib

# Whether the project is shared and online, its members, and who holds the Hotlink and XRef management.
# In a solo project isShared and isOnline are false and nothing else is returned.
aclib.RunTapirCommand ('GetTeamworkStatus')

# The Teamwork access rights of the current user. Without names all rights are returned.
aclib.RunTapirCommand ('GetTeamworkAccessRights', {
    'accessRights': ['LayersCreate', 'LibraryPartDeleteModify', 'HotlinkAndXrefManage']
})
