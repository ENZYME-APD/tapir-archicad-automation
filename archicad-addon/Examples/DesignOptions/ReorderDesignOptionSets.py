"""Creates two design option sets and moves the second before the first with ReorderDesignOptionSets.

Deletes both sets at the end. Requires Archicad 29 or later.
"""
import aclib

aclib.RunTapirCommand ('CreateDesignOptionSets', {'designOptionSets': ['Tapir Example Set 1', 'Tapir Example Set 2']})
sets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
setIds = {s['name']: s['designOptionSetId'] for s in sets}

aclib.PrintJson (aclib.RunTapirCommand ('ReorderDesignOptionSets', {
    'designOptionSets': [{
        'designOptionSetId': setIds['Tapir Example Set 2'],
        'beforeDesignOptionSetId': setIds['Tapir Example Set 1']
    }]
}))
aclib.RunTapirCommand ('GetDesignOptionSets')

aclib.RunTapirCommand ('DeleteDesignOptionSets', {
    'designOptionSets': [{'designOptionSetId': setIds[name]} for name in ('Tapir Example Set 1', 'Tapir Example Set 2')]
})
