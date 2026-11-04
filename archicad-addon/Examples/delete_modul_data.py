import aclib

# DeleteModulData deletes the module data with the given names; a name without module data gets a
# failed execution result. Deleting is not an undoable step.
aclib.RunTapirCommand ('StoreModulData', {
    'modulData': [
        {'name': 'tapir-example/delete', 'content': 'to be deleted'}
    ]
}, debug = False)

aclib.RunTapirCommand ('DeleteModulData', {
    'names': ['tapir-example/delete', 'tapir-example/missing']
})

aclib.RunTapirCommand ('GetModulData', {
    'names': ['tapir-example/delete']
})
