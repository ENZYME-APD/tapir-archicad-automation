import aclib

# The graphic override combination applied to the current (front) window, e.g. the floor plan.
# Both commands are available from Archicad 30; on older versions they return an error.
activeCombination = aclib.RunTapirCommand ('GetActiveGraphicalOverrideCombination')

if activeCombination is not None and 'error' not in activeCombination:
    # Apply a combination by name ...
    aclib.RunTapirCommand ('SetActiveGraphicalOverrideCombination', {
        'name': 'Cardboard Model'
    })
    aclib.RunTapirCommand ('GetActiveGraphicalOverrideCombination')

    # ... then restore the previous one by id. An empty response means no combination was applied.
    if 'combinationId' in activeCombination:
        aclib.RunTapirCommand ('SetActiveGraphicalOverrideCombination', {
            'combinationId': activeCombination['combinationId']
        })
    else:
        aclib.RunTapirCommand ('SetActiveGraphicalOverrideCombination', {
            'name': 'No Overrides'
        })
