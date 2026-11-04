"""Reads every classification of the Wall, Slab and Column tool defaults in one call with GetClassificationsOfToolDefaults."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('GetClassificationsOfToolDefaults', {'elementTypes': ['Wall', 'Slab', 'Column']}))
