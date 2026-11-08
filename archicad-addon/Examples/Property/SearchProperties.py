"""Searches the properties whose name or group contains "Fire" with SearchProperties, then the custom string properties."""
import aclib

aclib.PrintJson (aclib.RunTapirCommand ('SearchProperties', {'query': 'Fire', 'limit': 10}))

aclib.PrintJson (aclib.RunTapirCommand ('SearchProperties', {'propertyType': 'Custom', 'valueType': 'String', 'limit': 10}))
