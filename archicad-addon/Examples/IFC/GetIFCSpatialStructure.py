"""Gets the IfcProject, IfcSite and IfcBuilding of the project and the IFC story index of the first walls with GetIFCSpatialStructure.

Requires Archicad 28 or later.
"""
import aclib

walls = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Wall'})['elements'][:3]

aclib.PrintJson (aclib.RunTapirCommand ('GetIFCSpatialStructure', {'elements': walls}))
