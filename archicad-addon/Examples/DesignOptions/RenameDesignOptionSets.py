"""Creates a design option set and renames it with RenameDesignOptionSets.

Deletes the set at the end. Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
setId = next (s['designOptionSetId'] for s in sets if s['name'] == setName)

aclib.PrintJson (aclib.RunTapirCommand ('RenameDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': setId, 'name': 'Tapir Renamed Set'}]
}))
aclib.RunTapirCommand ('GetDesignOptionSets')

aclib.RunTapirCommand ('DeleteDesignOptionSets', {'designOptionSets': [{'designOptionSetId': setId}]})
