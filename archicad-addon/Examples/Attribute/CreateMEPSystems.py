"""Creates a Piping MEP System with its pens, fill, center line type and surfaces with CreateMEPSystems.

Uses the first fill, line type and surface of the project. Overwrites the MEP System of the same name when
the script is run again.
"""
import aclib

def FirstAttributeId (attributeType):
    return {'attributeId': aclib.RunTapirCommand ('GetAttributesByType', {'attributeType': attributeType})['attributes'][0]['attributeId']}

aclib.PrintJson (aclib.RunTapirCommand ('CreateMEPSystems', {
    'mepSystemDataArray': [{
        'name': 'Tapir Example Piping System',
        'domain': 'Piping',
        'contourPen': 3,
        'fillPen': 3,
        'fillBackgroundPen': 0,
        'centerLinePen': 3,
        'fillId': FirstAttributeId ('Fill'),
        'centerLineTypeId': FirstAttributeId ('Line'),
        'materialId': FirstAttributeId ('Surface'),
        'insulationMaterialId': FirstAttributeId ('Surface')
    }],
    'overwriteExisting': True
}))
