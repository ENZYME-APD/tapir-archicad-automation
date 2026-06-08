import aclib

designOptions = aclib.RunTapirCommand ('GetDesignOptions')['designOptions']
designOptionSets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
designOptionCombinations = aclib.RunTapirCommand ('GetDesignOptionCombinations')['designOptionCombinations']

elementsOfDesignOptions = aclib.RunTapirCommand ('GetElementsOfDesignOptions', {
        'designOptions': [{ 'designOptionId': designOption['designOptionId'] } for designOption in designOptions]
    })['elementsOfDesignOptions']

elements = aclib.RunCommand ('API.GetAllElements')['elements']
designOptionForElements = aclib.RunTapirCommand ('GetDesignOptionForElements', {
        'elements': elements
    })['designOptionForElements']
