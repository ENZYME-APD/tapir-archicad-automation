import sys
import aclib

# Duplicates a design option combination: creates a design option set with an
# option and a combination, duplicates the combination, lists the combinations
# (the copy has the same active options), then deletes everything it created.
# The design option commands need Archicad 29 or newer; on older versions the
# first command fails and the example stops.

setName = 'Tapir Duplicate Example Set'

createSetResult = aclib.RunTapirCommand ('CreateDesignOptionSets', {
        'designOptionSets': [setName]
    })
if createSetResult is None or 'error' in createSetResult or not createSetResult['executionResults'][0]['success']:
    print ('Could not create a design option set, this example needs Archicad 29 or newer.')
    sys.exit ()

designOptionSets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
setId = next (s['designOptionSetId'] for s in designOptionSets if s['name'] == setName)

createdOptions = aclib.RunTapirCommand ('CreateDesignOptions', {
        'designOptions': [
            {'name': 'Option A', 'id': 'A', 'ownerSetName': setName}
        ]
    })['designOptionIdsOrErrors']
optionIds = [o['designOptionId'] for o in createdOptions if 'designOptionId' in o]

createdCombinations = aclib.RunTapirCommand ('CreateDesignOptionCombinations', {
        'designOptionCombinations': [
            {'name': 'Tapir Example Combination', 'activeDesignOptions': [{'designOptionId': optionIds[0]}]}
        ]
    })['designOptionCombinationIdsOrErrors']
combinationIds = [c['designOptionCombinationId'] for c in createdCombinations if 'designOptionCombinationId' in c]

duplicatedCombinations = aclib.RunTapirCommand ('DuplicateDesignOptionCombinations', {
        'designOptionCombinations': [{'designOptionCombinationId': c} for c in combinationIds]
    })['designOptionCombinationIdsOrErrors']
duplicateIds = [c['designOptionCombinationId'] for c in duplicatedCombinations if 'designOptionCombinationId' in c]

aclib.RunTapirCommand ('GetDesignOptionCombinations')

aclib.RunTapirCommand ('DeleteDesignOptionCombinations', {
        'designOptionCombinations': [{'designOptionCombinationId': c} for c in combinationIds + duplicateIds]
    })
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
        'designOptionSets': [{'designOptionSetId': setId}]
    })
