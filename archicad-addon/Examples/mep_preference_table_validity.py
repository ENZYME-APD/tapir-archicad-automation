import aclib

# Requires Archicad 28 or newer.

# Which rows of the preference tables are valid, and which reference ids
# and (from Archicad 29) angles of the domain's sets are defined.
for domain in ('Piping', 'Ventilation'):
    response = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
            'domain': domain,
            'includeFittingTables': True
        })

    for table in response['tables']:
        invalidRows = [row['referenceId'] for row in table['rows'] if row.get ('isValid') is False]
        print (f"{domain} segment table {table.get ('name', '')}: {len (table['rows'])} rows, invalid referenceIds: {invalidRows}")

    for tableKind in ('elbowTables', 'branchTables', 'transitionTables'):
        for table in response.get (tableKind, []):
            invalidRowCount = len ([row for row in table['rows'] if row.get ('isValid') is False])
            print (f"{domain} {tableKind} {table['name']}: {len (table['rows'])} rows, {invalidRowCount} invalid")

    referenceSet = response['referenceSet']
    for item in referenceSet.get ('referenceIdDetails', []):
        print (f"{domain} reference set {referenceSet['name']}: referenceId={item['referenceId']} defined={item.get ('isDefined')}")

    if 'angleSet' in response:
        angleSet = response['angleSet']
        for item in angleSet.get ('angleDetails', []):
            print (f"{domain} angle set {angleSet['name']}: angle={item['angle']} defined={item.get ('isDefined')}")
