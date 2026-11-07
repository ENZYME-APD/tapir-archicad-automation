"""Changes the size, rotation and floor plan display of the skylights with ModifySkylights.

Needs at least one Skylight. Only the settings given change; CreateSkylights takes the same keys,
and GetDetailsOfElements reads them back.
"""
import aclib

skylights = aclib.RunTapirCommand ('GetElementsByType', {'elementType': 'Skylight'})['elements']
if not skylights:
    raise SystemExit ('Place a Skylight first.')

aclib.PrintJson (aclib.RunTapirCommand (
    'ModifySkylights', {
        'skylightsWithDetails': [
            {
                'elementId': skylight['elementId'],
                'width': 1.0,
                'height': 1.2,
                'azimuthAngle': 0.0,
                'displayOption': 'OutLinesOnly',
                'pen': 7
            } for skylight in skylights
        ]
    }))

aclib.PrintJson (aclib.RunTapirCommand ('GetDetailsOfElements', {'elements': skylights}))
