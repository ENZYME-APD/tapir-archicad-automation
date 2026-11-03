import aclib

# Requires Archicad 28 or newer.

# Read, change, add and delete rows of the pipe branch, elbow, segment and transition
# preference tables by their key. From Archicad 29 the key has an angle (branchAxisAngle / angle, as
# GetMEPPreferenceTables returns it); Archicad 28 finds the rows by their reference ids only.
fittingTables = aclib.RunTapirCommand ('GetMEPPreferenceTables', {
        'domain': 'Piping',
        'includeFittingTables': True
    })

def BranchKey (row):
    key = {'mainAxisReferenceId': row['mainAxisReferenceId'], 'branchAxisReferenceId': row['branchAxisReferenceId']}
    if 'branchAxisAngle' in row:
        key['branchAxisAngle'] = row['branchAxisAngle']
    return key

def ElbowKey (row):
    key = {'referenceId': row['referenceId']}
    if 'angle' in row:
        key['angle'] = row['angle']
    return key

branchTable = next ((t for t in fittingTables.get ('branchTables', []) if t['rows']), None)
if branchTable:
    tableId = {'guid': branchTable['guid']}
    key = BranchKey (branchTable['rows'][0])

    rows = aclib.RunTapirCommand ('GetMEPPreferenceTableRows', {
            'tableId': tableId,
            'keys': [key]
        })['rows']
    print (f"Branch table {branchTable['name']}, row {key}: {rows[0]}")

    # Only the given values are changed.
    result = aclib.RunTapirCommand ('SetMEPPreferenceTableRows', {
            'tableId': tableId,
            'rows': [{'key': key, 'values': {'description': 'Changed by Tapir'}}]
        })
    print (f"Set: {result['executionResults']}")

elbowTable = next ((t for t in fittingTables.get ('elbowTables', []) if t['rows']), None)
if elbowTable:
    tableId = {'guid': elbowTable['guid']}
    key = ElbowKey (elbowTable['rows'][0])

    rows = aclib.RunTapirCommand ('GetMEPPreferenceTableRows', {
            'tableId': tableId,
            'keys': [key]
        })['rows']
    print (f"Elbow table {elbowTable['name']}, row {key}: {rows[0]}")

    # Adding an elbow row needs Archicad 29 (the key needs the angle). The new row
    # gets an angle no row of the table has yet, then it is deleted again.
    if 'angle' in key:
        newKey = {'referenceId': key['referenceId'], 'angle': 7.5}
        added = aclib.RunTapirCommand ('AddMEPPreferenceTableRows', {
                'tableId': tableId,
                'rows': [{'key': newKey, 'values': {'description': 'Added by Tapir'}}]
            })
        print (f"Add: {added['executionResults']}")

        deleted = aclib.RunTapirCommand ('DeleteMEPPreferenceTableRows', {
                'tableId': tableId,
                'keys': [newKey]
            })
        print (f"Delete: {deleted['executionResults']}")

# Segment tables have a row for each reference id, which is their key.
segmentTable = next ((t for t in fittingTables['tables'] if t['rows']), None)
if segmentTable:
    tableId = {'guid': segmentTable['guid']}
    key = {'referenceId': segmentTable['rows'][0]['referenceId']}

    result = aclib.RunTapirCommand ('GetMEPPreferenceTableRows', {
            'tableId': tableId,
            'keys': [key]
        })
    print (f"Segment table {segmentTable['guid']} exists: {result['exists']}, row {key}: {result['rows'][0]}")

    result = aclib.RunTapirCommand ('SetMEPPreferenceTableRows', {
            'tableId': tableId,
            'rows': [{'key': key, 'values': {'description': 'Changed by Tapir'}}]
        })
    print (f"Set: {result['executionResults']}")

# Transition tables need Archicad 29; their key is the begin and end reference id.
transitionTable = next ((t for t in fittingTables.get ('transitionTables', []) if t['rows']), None)
if transitionTable:
    tableId = {'guid': transitionTable['guid']}
    row = transitionTable['rows'][0]
    key = {'beginReferenceId': row['beginReferenceId'], 'endReferenceId': row['endReferenceId']}

    rows = aclib.RunTapirCommand ('GetMEPPreferenceTableRows', {
            'tableId': tableId,
            'keys': [key]
        })['rows']
    print (f"Transition table {transitionTable['name']}, row {key}: {rows[0]}")

    # Pipe transitions have a length, duct transitions an angle.
    result = aclib.RunTapirCommand ('SetMEPPreferenceTableRows', {
            'tableId': tableId,
            'rows': [{'key': key, 'values': {'description': 'Changed by Tapir'}}]
        })
    print (f"Set: {result['executionResults']}")
