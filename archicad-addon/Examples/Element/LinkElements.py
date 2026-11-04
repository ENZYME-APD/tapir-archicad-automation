"""Links the first wall to the second one with LinkElements, reads the link back, then removes it.

Scripts can record their own relations between elements this way. Needs at least two Walls.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:2]
if len (walls) < 2:
    raise SystemExit ('Fewer than two walls in the project.')

# flags is a 32-bit integer free for the script's own use.
aclib.PrintJson (aclib.RunTapirCommand ('LinkElements', {
    'links': [{'fromElementId': walls[0]['elementId'], 'toElementId': walls[1]['elementId'], 'flags': 1}]
}))
aclib.RunTapirCommand ('GetLinksOfElements', {'elements': walls[:1]})

aclib.RunTapirCommand ('UnlinkElements', {
    'links': [{'fromElementId': walls[0]['elementId'], 'toElementId': walls[1]['elementId']}]
})
