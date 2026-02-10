import aclib

designOptions = aclib.RunTapirCommand ('GetDesignOptions')['designOptions']
designOptionSets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
designOptionCombinations = aclib.RunTapirCommand ('GetDesignOptionCombinations')['designOptionCombinations']