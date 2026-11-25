"""Prints the pattern entry of each segment of the first railing of the project with GetRailingSegmentPatterns,
and the pattern instances Archicad generated for it.

The entry is the segment's pattern settings (its distribution and its panel, baluster set and inner post
settings); the instances are subelements, whose own settings GetDetailsOfElements reads. Needs a railing.
"""
import aclib

railings = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Railing'})['elements']
if not railings:
    raise SystemExit ('No railing in the project.')

result = aclib.RunTapirCommand ('GetRailingSegmentPatterns', {'elements': railings[:1]})
aclib.PrintJson (result)

for segment in result['railings'][0].get ('segments', []):
    spacing = segment['entry']['balusterSet']['distribution']
    print ('segment {}: baluster distribution {} (length {}), {} pattern instance(s)'.format (
        segment['segmentIndex'], spacing['type'], spacing['length'], len (segment['instances']['patterns'])))
