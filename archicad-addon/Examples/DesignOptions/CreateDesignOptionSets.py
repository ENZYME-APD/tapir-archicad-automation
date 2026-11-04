"""Creates a design option set with CreateDesignOptionSets, then deletes it again.

Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.PrintJson (aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]}))

sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
})
