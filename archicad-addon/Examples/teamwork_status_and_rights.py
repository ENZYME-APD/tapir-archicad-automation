import aclib

# Whether the project is shared and online, its members, and who holds the Hotlink and XRef management.
# In a solo project isShared and isOnline are false and nothing else is returned.
aclib.RunTapirCommand ('GetTeamworkStatus')

# The Teamwork access rights of the current user. Without names all rights are returned.
aclib.RunTapirCommand ('GetTeamworkAccessRights', {
    'accessRights': ['LayersCreate', 'LibraryPartDeleteModify', 'HotlinkAndXrefManage']
})

# Whether the current user may create objects of the kind of each wall and delete or modify it.
walls = aclib.RunCommand ('API.GetElementsByType', { 'elementType' : 'Wall' })['elements']
aclib.RunTapirCommand ('GetTeamworkRightsOfObjects', {
    'objectIds' : [element['elementId'] for element in walls[:3]]
})

# The Teamwork owner of the first three layouts. userId is 0 when nobody owns the layout.
layouts = aclib.RunTapirCommand ('GetDatabases', {'types': ['Layout']}, debug=False)['databases']
if layouts:
    aclib.RunTapirCommand ('GetTeamworkOwnersOfDatabases', {
        'databases': [{'databaseId': layout['databaseId']} for layout in layouts[:3]]
    })
