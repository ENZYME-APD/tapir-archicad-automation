import aclib

# Requires Archicad 28 or newer.

# List the circular cross section preference tables of the Piping domain
# (referenceId, diameter and description per row). The referenceId of a row
# can be passed as crossSectionReferenceId to CreateMEPRoutingElements /
# ModifyMEPRoutingElements to pick that exact cross section.
pipeTables = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
        'domain': 'Piping'
    })['tables']

for table in pipeTables:
    print (f"Pipe preference table {table['guid']}:")
    for row in table['rows']:
        description = row.get ('description', '')
        print (f"  referenceId={row['referenceId']} diameter={row['diameter']} {description}")

# Same for the Ventilation (duct) domain
ductTables = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
        'domain': 'Ventilation'
    })['tables']

for table in ductTables:
    print (f"Duct preference table {table['guid']}:")
    for row in table['rows']:
        description = row.get ('description', '')
        print (f"  referenceId={row['referenceId']} diameter={row['diameter']} {description}")
