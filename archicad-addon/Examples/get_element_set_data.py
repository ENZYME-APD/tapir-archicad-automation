import sys
import base64
import aclib

# Pass the guids of the element sets as command line arguments.
# Element sets are created by add-ons to link elements together.
setGuids = aclib.unknownArgs
if not setGuids:
    print('Usage: python get_element_set_data.py <element set guid> [<element set guid> ...]')
    sys.exit(0)

response = aclib.RunTapirCommand(
    'GetElementSetData', {
        'elementSets': [{'elementSetId': {'guid': guid}} for guid in setGuids]
    })

if response is not None:
    for guid, elementSet in zip(setGuids, response['elementSets']):
        if 'error' in elementSet:
            continue
        data = base64.b64decode(elementSet['userData']['data'])
        print(f"{guid}: {len(elementSet['elements'])} elements, {len(data)} bytes of data")
