"""Reads the default design option with GetDefaultDesignOption, before and after making a new option the default.

Creates a design option set with one option, puts the default back as it was and deletes the set at the end.
Requires Archicad 29 or later.
"""
import aclib

before = aclib.RunTapirCommand ('GetDefaultDesignOption')
aclib.PrintJson (before)

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
option = aclib.RunTapirCommand ('CreateDesignOptions', {
    'designOptions': [{'name': 'Option A', 'id': 'A', 'ownerSetName': setName}]
})['designOptionIdsOrErrors'][0]

aclib.RunTapirCommand ('SetDefaultDesignOption', {'designOptionId': option['designOptionId']})
aclib.PrintJson (aclib.RunTapirCommand ('GetDefaultDesignOption'))

# The default as it was: the Main Model, or the design option read at the start.
aclib.RunTapirCommand ('SetDefaultDesignOption', {} if before['isMainModel'] else {'designOptionId': before['designOption']['designOptionId']})

sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
