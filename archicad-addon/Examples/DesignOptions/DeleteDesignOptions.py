"""Creates a design option set with an option, then deletes the option with DeleteDesignOptions.

Deletes the set too at the end. Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
option = aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [{'name': 'Option A', 'id': 'A', 'ownerSetName': setName}]
})['designOptionIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('DeleteDesignOptions', {'designOptions': [{'designOptionId': option['designOptionId']}]}))
aclib.RunTapirCommand ('GetDesignOptions')

sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
