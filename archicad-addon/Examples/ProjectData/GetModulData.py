"""GetModulData returns the module data Tapir stored in the project (StoreModulData): every one without names, or
the given names. With includeContent false only the names and data versions are read. A name without module
data gets an error item.
"""
import aclib

aclib.RunTapirCommand ('StoreModulData', {
    'modulData': [
        {'name': 'tapir-example/get', 'content': '{"answer": 42}'}
    ]
})

aclib.PrintJson (aclib.RunTapirCommand ('GetModulData', {
    'includeContent': False
}))

aclib.PrintJson (aclib.RunTapirCommand ('GetModulData', {
    'names': ['tapir-example/get', 'tapir-example/missing']
}))

aclib.RunTapirCommand ('DeleteModulData', {
    'names': ['tapir-example/get']
})
