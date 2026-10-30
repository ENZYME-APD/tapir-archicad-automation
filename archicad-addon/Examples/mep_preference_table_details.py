import aclib

# Requires Archicad 28 or newer.

# The name, the editable flag and (from Archicad 29) the segmentation lengths
# of every segment preference table, including the rectangular duct tables
# and the cable carrier tables. Sizes are in meters.
def PrintTableHeader (kind, table):
    print (f"{kind} table {table.get ('name', '')} ({table['guid']}), editable: {table.get ('isEditable')}")
    if 'useIndividualSegmentLengths' in table:
        print (f"  individual segment lengths: {table['useIndividualSegmentLengths']}, "
               f"segmentation length: {table.get ('minimumSegmentationLength')} - {table.get ('maximumSegmentationLength')}")

for domain in ('Piping', 'Ventilation'):
    response = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
            'domain': domain,
            'includeFittingTables': True
        })

    for table in response['tables']:
        PrintTableHeader (f"{domain} circular", table)
        for row in table['rows']:
            print (f"  referenceId={row['referenceId']} diameter={row['diameter']} wallThickness={row.get ('wallThickness')} {row.get ('description', '')}")

    for table in response.get ('rectangularTables', []):
        PrintTableHeader (f"{domain} rectangular", table)
        print (f"  sizes: {[row['value'] for row in table['rows']]}")

    referenceSet = response['referenceSet']
    print (f"{domain} reference set {referenceSet['name']}, editable: {referenceSet.get ('isEditable')}")
    if 'angleSet' in response:
        print (f"{domain} angle set {response['angleSet']['name']}, editable: {response['angleSet'].get ('isEditable')}")
    for tableKind in ('elbowTables', 'branchTables', 'transitionTables'):
        for table in response.get (tableKind, []):
            print (f"{domain} {tableKind} {table['name']}, editable: {table.get ('isEditable')}")

cableCarrierTables = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
        'domain': 'CableCarrier'
    })['cableCarrierTables']

for table in cableCarrierTables:
    PrintTableHeader ('CableCarrier', table)
    for row in table['rows']:
        print (f"  value={row['value']} {row.get ('description', '')}")
