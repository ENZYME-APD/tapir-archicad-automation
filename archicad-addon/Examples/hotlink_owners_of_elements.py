import aclib

# Finding out which elements come from a hotlinked module.
#
# GetHotlinkOwnersOfElements tells for each element whether it is part of a
# hotlink, and returns the hotlink element that contains it and the hotlink
# instance it belongs to. A script that moves, changes or deletes elements can
# use it to skip the hotlinked ones up front.

elements = aclib.RunCommand ('API.GetAllElements', {})['elements']

hotlinkOwners = aclib.RunTapirCommand ('GetHotlinkOwnersOfElements', {
    'elements': elements
})['hotlinkOwners']

hotlinkedElements = [e for e, owners in zip (elements, hotlinkOwners) if owners.get ('isInHotlink', False)]
print (f'{len (hotlinkedElements)} of {len (elements)} elements come from a hotlink.')
