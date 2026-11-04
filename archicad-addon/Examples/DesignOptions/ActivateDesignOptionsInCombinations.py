"""Creates an empty combination and activates a design option in it with ActivateDesignOptionsInCombinations.

Deletes everything it created at the end. Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
option = aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [{'name': 'Option A', 'id': 'A', 'ownerSetName': setName}]
})['designOptionIdsOrErrors'][0]
combination = aclib.RunTapirCommand ('CreateDesignOptionCombinations', {
    'designOptionCombinations': [{'name': 'Tapir Example Combination', 'activeDesignOptions': []}]
})['designOptionCombinationIdsOrErrors'][0]

# The other active options of the combination stay as they are.
aclib.PrintJson (aclib.RunTapirCommand ('ActivateDesignOptionsInCombinations', {
    'designOptionsToActivate': [{
        'designOptionCombinationId': combination['designOptionCombinationId'],
        'designOptionId': option['designOptionId']
    }]
}))
aclib.RunTapirCommand ('GetDesignOptionCombinations')

aclib.RunTapirCommand ('DeleteDesignOptionCombinations', {
    'designOptionCombinations': [{'designOptionCombinationId': combination['designOptionCombinationId']}]
})
sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
