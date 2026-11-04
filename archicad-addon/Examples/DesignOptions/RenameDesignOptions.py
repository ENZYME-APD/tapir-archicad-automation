"""Creates a design option and changes its name and ID with RenameDesignOptions.

Deletes the design option set it created at the end. Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
option = aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [{'name': 'Option A', 'id': 'A', 'ownerSetName': setName}]
})['designOptionIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('RenameDesignOptions', {
    'designOptions': [{'designOptionId': option['designOptionId'], 'name': 'Option A (renamed)', 'id': 'A2'}]
}))
aclib.RunTapirCommand ('GetDesignOptions')

sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
