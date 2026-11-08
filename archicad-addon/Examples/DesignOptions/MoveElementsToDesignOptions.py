"""Moves the first wall into a new design option with MoveElementsToDesignOptions, then back where it was.

Needs at least one Wall. Deletes the design option set it created at the end.
Requires Archicad 29 or later.
"""
import aclib

NullGuid = {'guid': '00000000-0000-0000-0000-000000000000'}

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:1]
if not walls:
    raise SystemExit ('No wall in the project.')
wallId = walls[0]['elementId']

# Remember the design option of the wall; NullGuid stands for the main model.
original = aclib.RunTapirCommand ('GetDesignOptionForElements', {'elements': walls})['designOptionForElements'][0]
originalOptionId = original['designOption']['designOptionId'] if 'designOption' in original else NullGuid

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
option = aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [{'name': 'Option A', 'id': 'A', 'ownerSetName': setName}]
})['designOptionIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('MoveElementsToDesignOptions', {
    'elementDesignOptionPairs': [{'elementId': wallId, 'designOptionId': option['designOptionId']}]
}))
aclib.RunTapirCommand ('GetDesignOptionForElements', {'elements': walls})

aclib.PrintJson (aclib.RunTapirCommand ('MoveElementsToDesignOptions', {
    'elementDesignOptionPairs': [{'elementId': wallId, 'designOptionId': originalOptionId}]
}))
sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
