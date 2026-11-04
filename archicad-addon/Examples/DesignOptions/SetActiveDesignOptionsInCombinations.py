"""Creates a combination that shows Option A, then makes it show Option B with SetActiveDesignOptionsInCombinations.

Deletes everything it created at the end. Requires Archicad 29 or later.
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
combination = aclib.RunTapirCommand ('CreateDesignOptionCombinations', {
    'designOptionCombinations': [{
        'name': 'Tapir Example Combination',
        'activeDesignOptions': [{'designOptionId': optionA['designOptionId']}]
    }]
})['designOptionCombinationIdsOrErrors'][0]

# The given list replaces the active options of the combination.
aclib.PrintJson (aclib.RunTapirCommand ('SetActiveDesignOptionsInCombinations', {
    'activeDesignOptionsInCombinations': [{
        'designOptionCombinationId': combination['designOptionCombinationId'],
        'activeDesignOptions': [{'designOptionId': optionB['designOptionId']}]
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
