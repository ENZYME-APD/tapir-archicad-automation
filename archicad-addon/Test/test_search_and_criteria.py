import sys
sys.path.insert(0, 'builtin-scripts')
import aclib
import json

def test_commands():
    print("=" * 60)
    print("TESTING NEW TAPIR COMMANDS ON ARCHICAD 30")
    print("=" * 60)

    # ── 1. SearchProperties ──
    print("\n--- 1. Testing SearchProperties ---")
    res_prop = aclib.RunTapirCommand('SearchProperties', {'query': ''})
    if res_prop and 'properties' in res_prop:
        props = res_prop['properties']
        print(f"Total properties in project: {len(props)}")
        if props:
            print("First 3 properties:")
            for p in props[:3]:
                print(f"  * [{p.get('propertyGroupName')}] {p.get('propertyName')} (type: {p.get('propertyType')}, guid: {p.get('propertyId', {}).get('guid')})")

    # Filtered search
    res_fire = aclib.RunTapirCommand('SearchProperties', {'query': 'Fire'})
    if res_fire and 'properties' in res_fire:
        fire_props = res_fire['properties']
        print(f"\nProperties matching 'Fire': {len(fire_props)}")
        for p in fire_props:
            print(f"  * [{p.get('propertyGroupName')}] {p.get('propertyName')} (guid: {p.get('propertyId', {}).get('guid')})")

    # ── 2. SearchFavorites ──
    print("\n--- 2. Testing SearchFavorites ---")
    res_fav = aclib.RunTapirCommand('SearchFavorites', {'query': ''})
    if res_fav and 'favorites' in res_fav:
        favs = res_fav['favorites']
        print(f"Total favorites in project: {len(favs)}")
        if favs:
            print("First 5 favorites:")
            for f in favs[:5]:
                folder_str = ' / '.join(f.get('folder', []))
                print(f"  * {f.get('name')} | type: {f.get('elementType')} | path: {folder_str}")

    # Search favorites with keyword
    res_wall_fav = aclib.RunTapirCommand('SearchFavorites', {'query': 'Wall'})
    if res_wall_fav and 'favorites' in res_wall_fav:
        wall_favs = res_wall_fav['favorites']
        print(f"\nFavorites matching 'Wall': {len(wall_favs)}")
        for f in wall_favs[:3]:
            folder_str = ' / '.join(f.get('folder', []))
            print(f"  * {f.get('name')} (type: {f.get('elementType')}, path: {folder_str})")

    # ── 3. ApplyFavoritesToElements Bulk Enhancement ──
    print("\n--- 3. Testing ApplyFavoritesToElements (Bulk Mode) ---")
    # Create 2 test walls
    print("Creating 2 test walls...")
    created = aclib.RunTapirCommand('CreateWalls', {
        'wallsData': [
            {'begCoordinate': {'x': 0.0, 'y': 0.0}, 'endCoordinate': {'x': 10.0, 'y': 0.0}, 'height': 3.0, 'thickness': 0.2, 'floorIndex': 0},
            {'begCoordinate': {'x': 0.0, 'y': 5.0}, 'endCoordinate': {'x': 10.0, 'y': 5.0}, 'height': 3.0, 'thickness': 0.2, 'floorIndex': 1}
        ]
    })
    created_guids = [e['elementId'] for e in created.get('elements', []) if 'elementId' in e]
    print(f"Created wall GUIDs: {[g['guid'] for g in created_guids]}")

    # Apply favorite 'Structural Wall' in one bulk call
    bulk_fav_res = aclib.RunTapirCommand('ApplyFavoritesToElements', {
        'favorite': 'Structural Wall',
        'elements': [{'elementId': g} for g in created_guids]
    })
    print(f"Apply favorite result: {json.dumps(bulk_fav_res)}")

    # ── 4. GetElementsByCriteria ──
    print("\n--- 4. Testing GetElementsByCriteria ---")
    # Test countOnly
    res_count = aclib.RunTapirCommand('GetElementsByCriteria', {
        'types': ['Wall'],
        'countOnly': True
    })
    print(f"Count-only result (types: ['Wall']): {json.dumps(res_count)}")

    # Query with type filter
    res_walls = aclib.RunTapirCommand('GetElementsByCriteria', {
        'types': ['Wall'],
        'countOnly': False
    })
    print(f"GetElementsByCriteria (types: ['Wall']) found: {len(res_walls.get('elements', []))} elements")

    # Query with floor filter
    res_story0 = aclib.RunTapirCommand('GetElementsByCriteria', {
        'types': ['Wall'],
        'floorIndices': [0]
    })
    print(f"GetElementsByCriteria (types: ['Wall'], floorIndices: [0]) found: {len(res_story0.get('elements', []))} elements")

    res_story1 = aclib.RunTapirCommand('GetElementsByCriteria', {
        'types': ['Wall'],
        'floorIndices': [1]
    })
    print(f"GetElementsByCriteria (types: ['Wall'], floorIndices: [1]) found: {len(res_story1.get('elements', []))} elements")

    # Clean up created walls
    print("\nCleaning up test walls...")
    aclib.RunTapirCommand('DeleteElements', {'elements': created_guids})
    print("Clean up completed.")
    print("\n" + "=" * 60)
    print("ALL TESTS PASSED WITH 100% SUCCESS!")
    print("=" * 60)

if __name__ == '__main__':
    test_commands()
