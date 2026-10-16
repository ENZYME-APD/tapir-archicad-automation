import sys
import aclib

# Manages design options by script: creates a design option set with two
# options and a combination, renames all three, then deletes everything it
# created. The design option commands need Archicad 29 or newer; on older
# versions the first command fails and the example stops.

setName = 'Tapir Example Set'

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
            {'name': 'Tapir Example Combination', 'activeDesignOptions': [{'designOptionId': optionIds[0]}]}
        ]
    })['designOptionCombinationIdsOrErrors']
combinationIds = [c['designOptionCombinationId'] for c in createdCombinations if 'designOptionCombinationId' in c]

# Rename: an option takes a new name and/or a new id, a set and a combination a new name.
aclib.RunTapirCommand ('RenameDesignOptions', {
        'designOptions': [
            {'designOptionId': optionIds[0], 'name': 'Option A (renamed)', 'id': 'A2'},
            {'designOptionId': optionIds[1], 'name': 'Option B (renamed)'}
        ]
    })
aclib.RunTapirCommand ('RenameDesignOptionSets', {
        'designOptionSets': [{'designOptionSetId': setId, 'name': setName + ' (renamed)'}]
    })
aclib.RunTapirCommand ('RenameDesignOptionCombinations', {
        'designOptionCombinations': [{'designOptionCombinationId': combinationIds[0], 'name': 'Tapir Example Combination (renamed)'}]
    })

aclib.RunTapirCommand ('GetDesignOptions')
aclib.RunTapirCommand ('GetDesignOptionCombinations')

# Delete: the combination, one option on its own, then the set with the option still in it.
aclib.RunTapirCommand ('DeleteDesignOptionCombinations', {
        'designOptionCombinations': [{'designOptionCombinationId': c} for c in combinationIds]
    })
aclib.RunTapirCommand ('DeleteDesignOptions', {
        'designOptions': [{'designOptionId': optionIds[1]}]
    })
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
        'designOptionSets': [{'designOptionSetId': setId}]
    })

aclib.RunTapirCommand ('GetDesignOptionSets')
