import sys
import aclib

# Sets the default design option and orders design option sets and the
# options within a set: creates two sets, three options in the first one,
# reorders them, sets an option and then the Main Model as the default, and
# finally deletes everything it created. The design option commands need
# Archicad 29 or newer; on older versions the first command fails and the
# example stops.

firstSetName = 'Tapir Example Set 1'
secondSetName = 'Tapir Example Set 2'

createSetResult = aclib.RunTapirCommand ('CreateDesignOptionSets', {
        'designOptionSets': [firstSetName, secondSetName]
    })
if createSetResult is None or 'error' in createSetResult or not createSetResult['executionResults'][0]['success']:
    print ('Could not create a design option set, this example needs Archicad 29 or newer.')
    sys.exit ()

designOptionSets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
firstSetId = next (s['designOptionSetId'] for s in designOptionSets if s['name'] == firstSetName)
secondSetId = next (s['designOptionSetId'] for s in designOptionSets if s['name'] == secondSetName)

createdOptions = aclib.RunTapirCommand ('CreateDesignOptions', {
        'designOptions': [
            {'name': 'Option A', 'id': 'A', 'ownerSetName': firstSetName},
            {'name': 'Option B', 'id': 'B', 'ownerSetName': firstSetName},
            {'name': 'Option C', 'id': 'C', 'ownerSetName': firstSetName}
        ]
    })['designOptionIdsOrErrors']
optionIds = [o['designOptionId'] for o in createdOptions if 'designOptionId' in o]

# Order the options of the first set: C before A, then B to the end (C, A, B).
aclib.RunTapirCommand ('ReorderDesignOptionsInSet', {
        'designOptions': [
            {'designOptionId': optionIds[2], 'beforeDesignOptionId': optionIds[0]},
            {'designOptionId': optionIds[1]}
        ]
    })

# Order the sets: the second set before the first one.
aclib.RunTapirCommand ('ReorderDesignOptionSets', {
        'designOptionSets': [
            {'designOptionSetId': secondSetId, 'beforeDesignOptionSetId': firstSetId}
        ]
    })

aclib.RunTapirCommand ('GetDesignOptionSets')

# Set Option A as the default, then the Main Model again (no designOptionId).
aclib.RunTapirCommand ('SetDefaultDesignOption', {
        'designOptionId': optionIds[0]
    })
aclib.RunTapirCommand ('SetDefaultDesignOption', {})

aclib.RunTapirCommand ('DeleteDesignOptionSets', {
        'designOptionSets': [{'designOptionSetId': firstSetId}, {'designOptionSetId': secondSetId}]
    })
