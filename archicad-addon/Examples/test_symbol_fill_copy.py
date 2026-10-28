"""
Live test (#1614): copy every Symbol fill of the open project with GetFills -> CreateFills and
compare the copy with the original, including the repeat vectors (symbolOffset1/2) and the
filled polygons (symbolSolidFills). The copies are deleted at the end. Draw a hatch with an
original and its copy before the cleanup to compare them by eye (set KEEP_COPIES = True).
"""
import aclib

KEEP_COPIES = False
SUFFIX = ' (Tapir copy test)'


def run(cmd, params=None):
    return aclib.RunTapirCommand(cmd, params or {}, debug=False)


def strip(fill):
    return {k: v for k, v in fill.items() if k not in ('attributeId', 'index', 'name')}


fills = run('GetAttributesByType', {'attributeType': 'Fill'})['attributes']
data = run('GetFills', {'attributeIds': [{'attributeId': f['attributeId']} for f in fills]})['fills']
symbols = [f for f in data if f.get('subType') == 'Symbol']
print(f'{len(symbols)} Symbol fills found')

passes = fails = 0
copies = []
for original in symbols:
    payload = dict(strip(original), name=original['name'] + SUFFIX)
    r = run('CreateFills', {'fillDataArray': [payload]})
    copy_id = r['attributeIds'][0].get('attributeId')
    if copy_id is None:
        fails += 1
        print(f"  [FAIL] {original['name']}: {r['attributeIds'][0]}")
        continue
    copies.append(copy_id)
    copy = run('GetFills', {'attributeIds': [{'attributeId': copy_id}]})['fills'][0]
    same = strip(copy) == strip(original)
    passes += same
    fails += not same
    print(f"  [{'PASS' if same else 'FAIL'}] {original['name']}: offsets {original.get('symbolOffset1')} / {original.get('symbolOffset2')}, "
          f"{len(original.get('symbolSolidFills', []))} solid polygons")
    if not same:
        for k in sorted(set(original) | set(copy)):
            if k not in ('attributeId', 'index', 'name') and original.get(k) != copy.get(k):
                print(f'      {k}: original={original.get(k)!r} copy={copy.get(k)!r}')

if copies and not KEEP_COPIES:
    run('DeleteAttributes', {'attributesToDelete': [{'attributeType': 'Fill', 'attributeId': {'attributeId': c}} for c in copies]})

print(f'\n{passes} passed, {fails} failed')
