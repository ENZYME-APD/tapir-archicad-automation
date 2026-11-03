import sys
import aclib

# Turns off a design option in a combination: creates a design option set with
# two options and a combination with both active, deactivates one of them,
# lists the combinations (only the other option stays active), then deletes
# everything it created. The design option commands need Archicad 29 or newer;
# on older versions the first command fails and the example stops.

setName = 'Tapir Deactivate Example Set'

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
            {'name': 'Option A', 'id': 'A', 'ownerSetName': setName},
            {'name': 'Option B', 'id': 'B', 'ownerSetName': setName}
        ]
    })['designOptionIdsOrErrors']
optionIds = [o['designOptionId'] for o in createdOptions if 'designOptionId' in o]

createdCombinations = aclib.RunTapirCommand ('CreateDesignOptionCombinations', {
        'designOptionCombinations': [
            {'name': 'Tapir Example Combination', 'activeDesignOptions': [{'designOptionId': o} for o in optionIds]}
        ]
    })['designOptionCombinationIdsOrErrors']
combinationIds = [c['designOptionCombinationId'] for c in createdCombinations if 'designOptionCombinationId' in c]

aclib.RunTapirCommand ('DeactivateDesignOptionsInCombinations', {
        'deactivatedDesignOptionsInCombinations': [
            {'designOptionCombinationId': combinationIds[0], 'designOptions': [{'designOptionId': optionIds[1]}]}
        ]
    })

aclib.RunTapirCommand ('GetDesignOptionCombinations')

aclib.RunTapirCommand ('DeleteDesignOptionCombinations', {
        'designOptionCombinations': [{'designOptionCombinationId': c} for c in combinationIds]
    })
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
        'designOptionSets': [{'designOptionSetId': setId}]
    })
