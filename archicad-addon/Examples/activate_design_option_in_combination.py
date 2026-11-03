import sys
import aclib

# Activates a design option in a combination: creates a design option set with
# two options and a combination with the first option active, activates the
# second option in the combination, then deletes everything it created. The
# design option commands need Archicad 29 or newer; on older versions the first
# command fails and the example stops.

setName = 'Tapir Activate Example Set'

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
            {'name': 'Tapir Activate Example Combination', 'activeDesignOptions': [{'designOptionId': optionIds[0]}]}
        ]
    })['designOptionCombinationIdsOrErrors']
combinationIds = [c['designOptionCombinationId'] for c in createdCombinations if 'designOptionCombinationId' in c]

aclib.RunTapirCommand ('GetDesignOptionCombinations')

aclib.RunTapirCommand ('ActivateDesignOptionsInCombinations', {
        'designOptionsToActivate': [
            {'designOptionCombinationId': combinationIds[0], 'designOptionId': optionIds[1]}
        ]
    })

aclib.RunTapirCommand ('GetDesignOptionCombinations')

aclib.RunTapirCommand ('DeleteDesignOptionCombinations', {
        'designOptionCombinations': [{'designOptionCombinationId': c} for c in combinationIds]
    })
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
        'designOptionSets': [{'designOptionSetId': setId}]
    })
