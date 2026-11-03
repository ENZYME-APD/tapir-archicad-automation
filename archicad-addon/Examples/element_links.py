import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', { 'elementType': 'Wall' })['elements']
if len (walls) < 2:
    print ('This example needs at least two walls in the project.')
    exit ()

fromElementId = walls[0]['elementId']
toElementIds = [wall['elementId'] for wall in walls[1:3]]

# Link the first wall to the next ones; the flags are free for the script's own use
response = aclib.RunTapirCommand ('LinkElements', {
    'links': [
        { 'fromElementId': fromElementId, 'toElementId': toElementId, 'flags': index + 1 }
        for index, toElementId in enumerate (toElementIds)
    ]
})

# The links start from the first wall only; the other walls have none pointing back
response = aclib.RunTapirCommand ('GetLinksOfElements', {
    'elements': walls[:3]
})

# Remove the links again
response = aclib.RunTapirCommand ('UnlinkElements', {
    'links': [
        { 'fromElementId': fromElementId, 'toElementId': toElementId }
        for toElementId in toElementIds
    ]
})

response = aclib.RunTapirCommand ('GetLinksOfElements', {
    'elements': walls[:3]
})
