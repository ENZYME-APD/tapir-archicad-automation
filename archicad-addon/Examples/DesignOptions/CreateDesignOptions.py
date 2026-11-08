"""Creates a design option set with two design options in it using CreateDesignOptions.

Deletes the set with its options at the end. Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})

aclib.PrintJson (aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [
        {'name': 'Option A', 'id': 'A', 'ownerSetName': setName},
        {'name': 'Option B', 'id': 'B', 'ownerSetName': setName}
    ]
}))
aclib.RunTapirCommand ('GetDesignOptions')

sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
