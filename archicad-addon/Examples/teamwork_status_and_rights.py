import aclib

# Whether the project is shared and online, its members, and who holds the Hotlink and XRef management.
# In a solo project isShared and isOnline are false and nothing else is returned.
aclib.RunTapirCommand ('GetTeamworkStatus')

# Whether the current user may create objects of the kind of each wall and delete or modify it.
walls = aclib.RunCommand ('API.GetElementsByType', { 'elementType' : 'Wall' })['elements']
aclib.RunTapirCommand ('GetTeamworkRightsOfObjects', {
    'objectIds' : [element['elementId'] for element in walls[:3]]
})
