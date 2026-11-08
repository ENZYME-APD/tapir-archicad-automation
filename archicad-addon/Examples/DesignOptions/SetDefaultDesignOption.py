"""Creates a design option and makes it the default with SetDefaultDesignOption, then sets the Main Model as the default.

Deletes the design option set it created at the end. Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
option = aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [{'name': 'Option A', 'id': 'A', 'ownerSetName': setName}]
})['designOptionIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('SetDefaultDesignOption', {'designOptionId': option['designOptionId']}))

# Without a designOptionId the Main Model becomes the default.
aclib.PrintJson (aclib.RunTapirCommand ('SetDefaultDesignOption', {}))

sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
