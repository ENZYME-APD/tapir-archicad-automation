import aclib

# Requires Archicad 28 or newer.

# List the circular cross section preference tables of the Piping domain
# (referenceId, diameter and description per row). The guid of a table can be
# passed as preferenceTableId and the referenceId of a row as
# crossSectionReferenceId to CreateMEPRoutingElements / ModifyMEPRoutingElements
# to pick that exact cross section (see mep_route_preference_table.py).
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

# The fitting tables (elbows, branches and transitions) and the reference and angle sets.
# Angles are in degrees, sizes in meters. The angle set and the transition tables need Archicad 29.
pipeFittings = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
        'domain': 'Piping',
        'includeFittingTables': True
    })

print (f"Pipe reference set {pipeFittings['referenceSet']['name']}: {pipeFittings['referenceSet']['referenceIds']}")
if 'angleSet' in pipeFittings:
    print (f"Pipe angle set {pipeFittings['angleSet']['name']}: {pipeFittings['angleSet']['angles']}")
for tableKind in ('elbowTables', 'branchTables', 'transitionTables'):
    for table in pipeFittings.get (tableKind, []):
        print (f"{tableKind} {table['name']}: {len (table['rows'])} rows")
