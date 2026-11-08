"""Creates a design option combination and renames it with RenameDesignOptionCombinations.

Deletes the combination at the end. Requires Archicad 29 or later.
"""
import aclib

combination = aclib.RunTapirCommand ('CreateDesignOptionCombinations', {
    'designOptionCombinations': [{'name': 'Tapir Example Combination', 'activeDesignOptions': []}]
})['designOptionCombinationIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('RenameDesignOptionCombinations', {
    'designOptionCombinations': [{
        'designOptionCombinationId': combination['designOptionCombinationId'],
        'name': 'Tapir Renamed Combination'
    }]
}))
aclib.RunTapirCommand ('GetDesignOptionCombinations')

aclib.RunTapirCommand ('DeleteDesignOptionCombinations', {
    'designOptionCombinations': [{'designOptionCombinationId': combination['designOptionCombinationId']}]
})
