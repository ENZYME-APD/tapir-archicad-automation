"""Creates a set with Options A and B and moves B before A with ReorderDesignOptionsInSet.

Deletes the set at the end. Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
optionA, optionB = aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [
        {'name': 'Option A', 'id': 'A', 'ownerSetName': setName},
        {'name': 'Option B', 'id': 'B', 'ownerSetName': setName}
    ]
})['designOptionIdsOrErrors']

# Without beforeDesignOptionId the option goes to the end of its set.
aclib.PrintJson (aclib.RunTapirCommand ('ReorderDesignOptionsInSet', {
    'designOptions': [{'designOptionId': optionB['designOptionId'], 'beforeDesignOptionId': optionA['designOptionId']}]
}))
aclib.RunTapirCommand ('GetDesignOptionSets')

sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
