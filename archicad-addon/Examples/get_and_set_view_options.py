import aclib

# The model view options and on-screen view options of the current plan window, and the
# index of the model view option combination it applies.
current = aclib.RunTapirCommand ('GetViewOptions')
print (current['modelViewOptionIndex'], current['viewOptions']['onScreenViewOptions'])

# Change one field, leaving all others as they are.
aclib.RunTapirCommand ('SetViewOptions', {
    'viewOptions': {'onScreenViewOptions': {'boldCutLines': not current['viewOptions']['onScreenViewOptions']['boldCutLines']}}
})

# Restore the original options, then switch back to the original combination.
aclib.RunTapirCommand ('SetViewOptions', {
    'modelViewOptionIndex': current['modelViewOptionIndex'],
    'viewOptions': current['viewOptions']
})
