import aclib

# Every design option, set and combination tells whether it can be renamed,
# deleted or changed now, and why not when it cannot (e.g. Teamwork reservation).
designOptions = aclib.RunTapirCommand ('GetDesignOptions')['designOptions']
designOptionSets = aclib.RunTapirCommand ('GetDesignOptionSets')['designOptionSets']
designOptionCombinations = aclib.RunTapirCommand ('GetDesignOptionCombinations')['designOptionCombinations']

for item in designOptions + designOptionSets + designOptionCombinations:
    if not item.get ('isEditable', True):
        print ('{} is not editable: {}'.format (item['name'], item['notEditableError']['message']))

# Ask first whether the elements can be moved to another design option (or to
# the main model), then move only those that can.
elements = aclib.RunCommand ('API.GetAllElements')['elements']
canMoveElements = aclib.RunTapirCommand ('CanMoveElementsToDesignOptions', {
        'elements': elements
    })['canMoveElements']

movableElements = [e for e in canMoveElements if e['canMove']]
print ('{} of {} elements can be moved to another design option.'.format (len (movableElements), len (canMoveElements)))
