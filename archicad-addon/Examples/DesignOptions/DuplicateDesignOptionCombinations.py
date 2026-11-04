"""Creates a design option combination and duplicates it with DuplicateDesignOptionCombinations.

Deletes both combinations at the end. Requires Archicad 29 or later.
"""
import aclib

combination = aclib.RunTapirCommand ('CreateDesignOptionCombinations', {
    'designOptionCombinations': [{'name': 'Tapir Example Combination', 'activeDesignOptions': []}]
})['designOptionCombinationIdsOrErrors'][0]

duplicates = aclib.RunTapirCommand ('DuplicateDesignOptionCombinations', {
    'designOptionCombinations': [{'designOptionCombinationId': combination['designOptionCombinationId']}]
})['designOptionCombinationIdsOrErrors']
aclib.PrintJson (duplicates)
aclib.RunTapirCommand ('GetDesignOptionCombinations')

aclib.RunTapirCommand ('DeleteDesignOptionCombinations', {
    'designOptionCombinations': [combination] + [d for d in duplicates if 'designOptionCombinationId' in d]
})
