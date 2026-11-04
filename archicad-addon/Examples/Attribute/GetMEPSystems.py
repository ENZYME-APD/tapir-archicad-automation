"""Reads the domain, pens, fill, line type and surfaces of every MEP System with GetMEPSystems."""
import aclib

mepSystems = aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': 'MEPSystem'}, debug = False)['attributes']

aclib.RunTapirCommand ('GetMEPSystems', {
    'attributeIds': [{'attributeId': mepSystem['attributeId']} for mepSystem in mepSystems]
})
