"""Reads the profile values of every wall, beam and column with a custom profile: the size of the stretched profile and the value of each profile modifier on that element.

The values are measured on the stretched profile each element uses, so a profile stretched by the
element, or a modifier the element changed, reads with the element's own value. Writing them is not
possible: the Archicad API has no function for an element's modifier values or stretch.
"""
import aclib

elements = []
for elementType in ['Wall', 'Beam', 'Column']:
    elements += aclib.RunTapirCommand ('GetElementsByType', {'elementType': elementType})['elements']

values = aclib.RunTapirCommand ('GetProfileValuesOfElements', {'elements': elements})['profileValuesOfElements']
aclib.PrintJson ([value for value in values if value.get ('profiles')])
