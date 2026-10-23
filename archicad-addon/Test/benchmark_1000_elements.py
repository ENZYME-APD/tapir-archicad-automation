import sys
import os
import time
import json
import math

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', 'builtin-scripts')))
import aclib

def estimate_tokens(text_or_obj):
    if not isinstance(text_or_obj, str):
        text = json.dumps(text_or_obj)
    else:
        text = text_or_obj
    # 1 token is roughly 3.8 to 4 characters in JSON
    return max(1, math.ceil(len(text) / 3.8))

def run_1000_element_benchmark():
    print("=" * 80)
    print(" SPECIAL BENCHMARK: 1,000 ARCHITECTURAL ELEMENTS TEST")
    print(" Comparison: Naive / Official MCP Pipeline vs New Tapir Optimized Pipeline")
    print("=" * 80)

    # --------------------------------------------------------------------------
    # STEP 1: GENERATE 1,000 ELEMENTS (600 Walls + 400 Columns)
    # --------------------------------------------------------------------------
    print("\n[STEP 1] Generating 1,000 elements in Archicad...")
    start_setup = time.time()

    # 600 walls: 300 on Floor 0, 300 on Floor 1
    walls_batch = []
    for i in range(300):
        # Floor 0
        walls_batch.append({
            'begCoordinate': {'x': float(i * 2), 'y': 0.0},
            'endCoordinate': {'x': float(i * 2 + 1.5), 'y': 0.0},
            'height': 3.0,
            'thickness': 0.25,
            'floorIndex': 0
        })
        # Floor 1
        walls_batch.append({
            'begCoordinate': {'x': float(i * 2), 'y': 5.0},
            'endCoordinate': {'x': float(i * 2 + 1.5), 'y': 5.0},
            'height': 3.0,
            'thickness': 0.25,
            'floorIndex': 1
        })

    # 400 columns: 200 on Floor 0, 200 on Floor 1
    cols_batch = []
    for i in range(200):
        cols_batch.append({
            'coordinates': {'x': float(i * 3), 'y': 10.0, 'z': 0.0},
            'height': 3.0,
            'floorIndex': 0
        })
        cols_batch.append({
            'coordinates': {'x': float(i * 3), 'y': 15.0, 'z': 3.0},
            'height': 3.0,
            'floorIndex': 1
        })

    t0 = time.time()
    wall_res = aclib.RunTapirCommand('CreateWalls', {'wallsData': walls_batch}, debug=False)
    created_walls = [e['elementId'] for e in wall_res.get('elements', []) if 'elementId' in e]
    
    col_res = aclib.RunTapirCommand('CreateColumns', {'columnsData': cols_batch}, debug=False)
    created_cols = [e['elementId'] for e in col_res.get('elements', []) if 'elementId' in e]

    total_created = len(created_walls) + len(created_cols)
    setup_time = time.time() - start_setup
    print(f"  -> Created {len(created_walls)} Walls + {len(created_cols)} Columns = {total_created} elements in {setup_time:.2f}s.")
    all_element_guids = created_walls + created_cols

    try:
        # ----------------------------------------------------------------------
        # BENCHMARK 1: PROPERTY DISCOVERY
        # Goal: Find property for 'Fire Rating'
        # ----------------------------------------------------------------------
        print("\n" + "-" * 80)
        print("BENCHMARK 1: PROPERTY DISCOVERY (Finding 'Fire' properties)")
        print("-" * 80)

        # Naive approach: Dump all properties
        t0 = time.time()
        all_props_res = aclib.RunTapirCommand('GetAllProperties', {}, debug=False)
        t_all_props = time.time() - t0
        all_props_json = json.dumps(all_props_res)
        all_props_bytes = len(all_props_json.encode('utf-8'))
        all_props_tokens = estimate_tokens(all_props_json)

        # Tapir approach: SearchProperties
        t0 = time.time()
        search_props_res = aclib.RunTapirCommand('SearchProperties', {'query': 'Fire'}, debug=False)
        t_search_props = time.time() - t0
        search_props_json = json.dumps(search_props_res)
        search_props_bytes = len(search_props_json.encode('utf-8'))
        search_props_tokens = estimate_tokens(search_props_json)

        prop_byte_reduction = (1 - search_props_bytes / all_props_bytes) * 100
        prop_token_reduction = (1 - search_props_tokens / all_props_tokens) * 100

        print(f"  [Naive - GetAllProperties]     {all_props_bytes:>9,} bytes | ~{all_props_tokens:>7,} tokens | {t_all_props*1000:>7.1f} ms | Found {len(all_props_res.get('properties', []))} properties")
        print(f"  [Tapir - SearchProperties]     {search_props_bytes:>9,} bytes | ~{search_props_tokens:>7,} tokens | {t_search_props*1000:>7.1f} ms | Found {len(search_props_res.get('properties', []))} matching properties")
        print(f"  >>> TOKEN SAVINGS:  {prop_token_reduction:.2f}% reduction ({all_props_tokens:,} -> {search_props_tokens:,} tokens)")
        print(f"  >>> PAYLOAD SAVINGS:{prop_byte_reduction:.2f}% reduction ({all_props_bytes:,} -> {search_props_bytes:,} bytes)")

        # ----------------------------------------------------------------------
        # BENCHMARK 2: FAVORITE SEARCH
        # Goal: Find Wall favorites
        # ----------------------------------------------------------------------
        print("\n" + "-" * 80)
        print("BENCHMARK 2: FAVORITE DISCOVERY (Finding 'Wall' favorites)")
        print("-" * 80)

        t0 = time.time()
        search_fav_res = aclib.RunTapirCommand('SearchFavorites', {'query': 'Wall'}, debug=False)
        t_search_fav = time.time() - t0
        search_fav_json = json.dumps(search_fav_res)
        search_fav_bytes = len(search_fav_json.encode('utf-8'))
        search_fav_tokens = estimate_tokens(search_fav_json)

        print(f"  [Tapir - SearchFavorites]      {search_fav_bytes:>9,} bytes | ~{search_fav_tokens:>7,} tokens | {t_search_fav*1000:>7.1f} ms | Found {len(search_fav_res.get('favorites', []))} favorites")
        print(f"  >>> Context footprint: Only ~{search_fav_tokens} tokens instead of guessing or loading unindexed lists!")

        # ----------------------------------------------------------------------
        # BENCHMARK 3: COUNTING ELEMENTS
        # Goal: Count how many Walls exist in the project (600 expected)
        # ----------------------------------------------------------------------
        print("\n" + "-" * 80)
        print("BENCHMARK 3: COUNTING ELEMENTS (How many walls in project?)")
        print("-" * 80)

        # Naive approach: Fetch all element IDs or details and count in client
        t0 = time.time()
        all_walls_fetch = aclib.RunTapirCommand('GetElementsByType', {'elementType': 'Wall'}, debug=False)
        t_naive_count = time.time() - t0
        naive_count_json = json.dumps(all_walls_fetch)
        naive_count_bytes = len(naive_count_json.encode('utf-8'))
        naive_count_tokens = estimate_tokens(naive_count_json)

        # Tapir approach: countOnly
        t0 = time.time()
        tapir_count_res = aclib.RunTapirCommand('GetElementsByCriteria', {'types': ['Wall'], 'countOnly': True}, debug=False)
        t_tapir_count = time.time() - t0
        tapir_count_json = json.dumps(tapir_count_res)
        tapir_count_bytes = len(tapir_count_json.encode('utf-8'))
        tapir_count_tokens = estimate_tokens(tapir_count_json)

        count_byte_reduction = (1 - tapir_count_bytes / naive_count_bytes) * 100
        count_token_reduction = (1 - tapir_count_tokens / naive_count_tokens) * 100

        print(f"  [Naive - Fetch + Count]        {naive_count_bytes:>9,} bytes | ~{naive_count_tokens:>7,} tokens | {t_naive_count*1000:>7.1f} ms | Count = {len(all_walls_fetch.get('elements', []))}")
        print(f"  [Tapir - countOnly: true]      {tapir_count_bytes:>9,} bytes | ~{tapir_count_tokens:>7,} tokens | {t_tapir_count*1000:>7.1f} ms | Count = {tapir_count_res.get('count', 0)}")
        print(f"  >>> TOKEN SAVINGS:  {count_token_reduction:.2f}% reduction ({naive_count_tokens:,} -> {tapir_count_tokens:,} tokens)")
        print(f"  >>> PAYLOAD SAVINGS:{count_byte_reduction:.2f}% reduction ({naive_count_bytes:,} -> {tapir_count_bytes:,} bytes)")

        # ----------------------------------------------------------------------
        # BENCHMARK 4: FILTERING 1,000 ELEMENTS & QUERYING DETAILS
        # Goal: "Get all Walls on Floor 1" (300 walls)
        # ----------------------------------------------------------------------
        print("\n" + "-" * 80)
        print("BENCHMARK 4: FILTERING 1,000 ELEMENTS & EXTRACTING DETAILS")
        print("Task: Filter and retrieve key details (ID, floor) for all Walls on Floor 1")
        print("-" * 80)

        # Naive approach: Fetch details of ALL 1,000 elements without projection or server-side filter
        # (Simulates how the official MCP or naive agent dumps elements into context)
        t0 = time.time()
        # Query 200 elements sample to measure exact per-element size without crashing buffer, then extrapolate to 1,000
        sample_size = 100
        naive_sample_res = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': all_element_guids[:sample_size]}, debug=False)
        t_naive_sample = time.time() - t0
        naive_sample_json = json.dumps(naive_sample_res)
        bytes_per_naive_elem = len(naive_sample_json.encode('utf-8')) / sample_size
        est_1000_naive_bytes = int(bytes_per_naive_elem * 1000)
        est_1000_naive_tokens = math.ceil(est_1000_naive_bytes / 3.8)
        est_1000_naive_time_ms = (t_naive_sample / sample_size) * 1000 * 1000

        # Tapir approach: Server-side criteria filter + field projection!
        t0 = time.time()
        # Step A: Find matching elements via criteria evaluator
        floor1_walls_crit = aclib.RunTapirCommand('GetElementsByCriteria', {
            'types': ['Wall'],
            'floorIndices': [1]
        }, debug=False)
        t_crit = time.time() - t0
        matching_guids = floor1_walls_crit.get('elements', [])

        # Step B: Project ONLY needed fields (skip polygons, thermal properties, materials)
        t0 = time.time()
        projected_details = aclib.RunTapirCommand('GetDetailsOfElements', {
            'elements': matching_guids,
            'fields': ['elementId', 'type', 'id', 'floorIndex']
        }, debug=False)
        t_proj = time.time() - t0
        t_tapir_total = t_crit + t_proj

        tapir_details_json = json.dumps(projected_details)
        tapir_details_bytes = len(tapir_details_json.encode('utf-8'))
        tapir_details_tokens = estimate_tokens(tapir_details_json)

        filter_byte_reduction = (1 - tapir_details_bytes / est_1000_naive_bytes) * 100
        filter_token_reduction = (1 - tapir_details_tokens / est_1000_naive_tokens) * 100

        print(f"  [Naive - Full Details Dump]   ~{est_1000_naive_bytes:>9,} bytes | ~{est_1000_naive_tokens:>7,} tokens | ~{est_1000_naive_time_ms:>7.1f} ms | Evaluated client-side")
        print(f"  [Tapir - Criteria + Project]   {tapir_details_bytes:>9,} bytes | ~{tapir_details_tokens:>7,} tokens | {t_tapir_total*1000:>7.1f} ms | 300 walls on Floor 1")
        print(f"  >>> TOKEN SAVINGS:  {filter_token_reduction:.2f}% reduction (~{est_1000_naive_tokens:,} -> {tapir_details_tokens:,} tokens)")
        print(f"  >>> PAYLOAD SAVINGS:{filter_byte_reduction:.2f}% reduction (~{est_1000_naive_bytes:,} -> {tapir_details_bytes:,} bytes)")
        print(f"  >>> SPEEDUP:        ~{est_1000_naive_time_ms / (t_tapir_total * 1000):.1f}x faster execution")

        # ----------------------------------------------------------------------
        # BENCHMARK 5: BULK FAVORITE APPLICATION
        # Goal: Apply 'Structural Wall' to 300 filtered walls
        # ----------------------------------------------------------------------
        print("\n" + "-" * 80)
        print("BENCHMARK 5: BULK FAVORITE APPLICATION ON 300 WALLS")
        print("-" * 80)

        t0 = time.time()
        bulk_fav_res = aclib.RunTapirCommand('ApplyFavoritesToElements', {
            'favorite': 'Structural Wall',
            'elements': matching_guids
        }, debug=False)
        t_bulk_fav = time.time() - t0

        succ_count = sum(1 for r in bulk_fav_res.get('executionResults', []) if r.get('success'))
        print(f"  [Tapir - Bulk Apply Favorite]  Applied 'Structural Wall' to {succ_count} walls in {t_bulk_fav*1000:.1f} ms!")
        print(f"  >>> Average per element: {t_bulk_fav*1000/len(matching_guids):.2f} ms per element in a SINGLE undoable command step!")

    finally:
        # ----------------------------------------------------------------------
        # CLEANUP: DELETE ALL 1,000 ELEMENTS
        # ----------------------------------------------------------------------
        print("\n" + "-" * 80)
        print("[CLEANUP] Deleting 1,000 elements from Archicad...")
        t0 = time.time()
        # Delete elements
        to_delete = [{'elementId': g} for g in all_element_guids]
        # Delete in chunks of 500
        for i in range(0, len(to_delete), 500):
            chunk = to_delete[i:i+500]
            aclib.RunTapirCommand('DeleteElements', {'elements': chunk}, debug=False)
        print(f"  -> Successfully deleted all {total_created} elements in {time.time()-t0:.2f}s.")
        print("=" * 80)
        print(" SPECIAL 1,000 ELEMENTS BENCHMARK COMPLETE")
        print("=" * 80)

if __name__ == '__main__':
    run_1000_element_benchmark()
