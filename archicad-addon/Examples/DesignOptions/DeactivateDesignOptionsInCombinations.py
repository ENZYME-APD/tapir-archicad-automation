"""Creates a combination that shows a design option, then turns that option off with DeactivateDesignOptionsInCombinations.

Deletes everything it created at the end. Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
option = aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [{'name': 'Option A', 'id': 'A', 'ownerSetName': setName}]
})['designOptionIdsOrErrors'][0]
combination = aclib.RunTapirCommand ('CreateDesignOptionCombinations', {
    'designOptionCombinations': [{
        'name': 'Tapir Example Combination',
        'activeDesignOptions': [{'designOptionId': option['designOptionId']}]
    }]
})['designOptionCombinationIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('DeactivateDesignOptionsInCombinations', {
    'deactivatedDesignOptionsInCombinations': [{
        'designOptionCombinationId': combination['designOptionCombinationId'],
        'designOptions': [{'designOptionId': option['designOptionId']}]
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
