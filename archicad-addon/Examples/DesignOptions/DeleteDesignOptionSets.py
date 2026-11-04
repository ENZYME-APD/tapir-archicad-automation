"""Creates a design option set and deletes it with DeleteDesignOptionSets.

Deleting a set deletes its design options too. Requires Archicad 29 or later.
"""
import aclib

setName = 'Tapir Example Set'
aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': [setName]})
sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']

aclib.PrintJson (aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': s['designOptionSetId']} for s in sets if s['name'] == setName]
}))
