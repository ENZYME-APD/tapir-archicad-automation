"""Creates two design option sets and moves a design option from the first to the second with MoveDesignOptionsToAnotherSet.

Deletes both sets at the end. Requires Archicad 29 or later.
"""
import aclib

aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': ['Tapir Example Set 1', 'Tapir Example Set 2']})
option = aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [{'name': 'Option A', 'id': 'A', 'ownerSetName': 'Tapir Example Set 1'}]
})['designOptionIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('MoveDesignOptionsToAnotherSet', {
    'designOptionAndSetPairs': [{'designOptionId': option['designOptionId'], 'setName': 'Tapir Example Set 2'}]
}))
aclib.RunTapirCommand ('GetDesignOptions')

sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'].startswith ('Tapir Example Set')]
})
