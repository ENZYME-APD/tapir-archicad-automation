"""Lists the fields GetElementFields and SetElementFields know for angle dimensions, with their types.

Needs nothing in the project.
"""
import aclib

definitions = aclib.RunTapirCommand ('GetElementFieldDefinitions', {'elementType': 'AngleDimension'})
print (definitions['structName'], len (definitions['fields']), 'fields')
aclib.PrintJson (definitions['fields'][:10])
