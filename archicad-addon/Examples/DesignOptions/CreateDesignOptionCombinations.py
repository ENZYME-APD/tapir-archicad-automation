"""Creates a design option set with an option, and a combination that shows it, using CreateDesignOptionCombinations.

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
aclib.PrintJson (combination)

aclib.RunTapirCommand ('DeleteDesignOptionCombinations', {
    'designOptionCombinations': [{'designOptionCombinationId': combination['designOptionCombinationId']}]
})
sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
