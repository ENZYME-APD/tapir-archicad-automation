"""Creates a design option combination and deletes it with DeleteDesignOptionCombinations.

Requires Archicad 29 or later.
"""
import aclib

combination = aclib.RunTapirCommand ('CreateDesignOptionCombinations', {
    'designOptionCombinations': [{'name': 'Tapir Example Combination', 'activeDesignOptions': []}]
})['designOptionCombinationIdsOrErrors'][0]

aclib.PrintJson (aclib.RunTapirCommand ('DeleteDesignOptionCombinations', {
    'designOptionCombinations': [{'designOptionCombinationId': combination['designOptionCombinationId']}]
}))
