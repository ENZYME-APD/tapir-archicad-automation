import aclib

# Requires Archicad 27 or newer. The Archicad API does not document the layout of the report, so
# the command passes it on as it comes: parsed into usedAttributes when it is a JSON object,
# otherwise as text in raw.

result = aclib.RunTapirCommand ('GetUsedAttributes')
if result is not None and 'raw' in result:
    print ('The report is not a JSON object:')
    print (result['raw'])
