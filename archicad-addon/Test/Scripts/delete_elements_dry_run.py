"""DeleteElements with dryRun (#2195): previews the deletion of every wall and column of the
project, checks that the project is unchanged afterwards, then deletes them for real.

Prints only counts and results, no guids, so the output can be recorded as a golden."""
import aclib


def GetAllElementGuids ():
    return sorted (e['elementId']['guid'] for e in aclib.RunTapirCommand ('GetAllElements', {}, debug=False)['elements'])


walls = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Wall'})['elements']
columns = aclib.RunCommand ('API.GetElementsByType', {'elementType': 'Column'})['elements']
missing = [{'elementId': {'guid': '00000000-0000-0000-0000-000000000001'}}]
targets = walls + columns + missing

before = GetAllElementGuids ()

preview = aclib.RunTapirCommand ('DeleteElements', {'elements': targets, 'dryRun': True}, debug=False)
wouldDelete = sum (1 for r in preview['executionResults'] if r.get ('wouldDelete'))
previewFailed = [r['error']['message'] for r in preview['executionResults'] if not r['success']]

after = GetAllElementGuids ()

print ('targets:', len (targets))
print ('dry run flag in the response:', preview.get ('dryRun'))
print ('results:', len (preview['executionResults']))
print ('would delete:', wouldDelete)
print ('would fail:', previewFailed)
print ('project unchanged by the dry run:', before == after)
assert preview.get ('dryRun') is True
assert len (preview['executionResults']) == len (targets)
assert before == after

result = aclib.RunTapirCommand ('DeleteElements', {'elements': targets}, debug=False)
deleted = sum (1 for r in result['executionResults'] if r['success'])
afterDelete = GetAllElementGuids ()

print ('real run has no dry run flag:', 'dryRun' not in result)
print ('real run has no wouldDelete:', all ('wouldDelete' not in r for r in result['executionResults']))
print ('deleted:', deleted)
print ('as previewed:', deleted == wouldDelete)
print ('elements removed from the project:', len (before) - len (afterDelete))
assert 'dryRun' not in result
assert deleted == wouldDelete
