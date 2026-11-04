"""Reads the domain, pens, fill, line type and surfaces of every MEP System with GetMEPSystems."""
import aclib

mepSystems = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'MEPSystem'})['attributes']

aclib.PrintJson (aclib.RunTapirCommand ('GetMEPSystems', {
    'attributeIds': [{'attributeId': mepSystem['attributeId']} for mepSystem in mepSystems]
}))
