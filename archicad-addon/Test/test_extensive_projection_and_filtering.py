import sys
import os
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'Examples')))

import aclib
import time
import json

def run_test():
    print("=" * 70)
    print(" EXTENSIVE TEST: SERVER-SIDE FILTERING & PROJECTION BENCHMARK")
    print("=" * 70)

    created_guids = []

    try:
        # ── 1. SETUP: Create 10 Walls and 6 Columns across Story 0 and Story 1 ──
        print("\n[STEP 1] Generating test elements...")
        
        # 5 Walls on Story 0, 5 Walls on Story 1
        walls_data = []
        for i in range(5):
            walls_data.append({
                'begCoordinate': {'x': 0.0, 'y': float(i * 3)},
                'endCoordinate': {'x': 8.0, 'y': float(i * 3)},
                'height': 3.0,
                'thickness': 0.25,
                'floorIndex': 0
            })
            walls_data.append({
                'begCoordinate': {'x': 10.0, 'y': float(i * 3)},
                'endCoordinate': {'x': 18.0, 'y': float(i * 3)},
                'height': 3.0,
                'thickness': 0.25,
                'floorIndex': 1
            })

        wall_res = aclib.RunTapirCommand('CreateWalls', {'wallsData': walls_data}, debug=False)
        wall_guids = [e['elementId'] for e in wall_res.get('elements', []) if 'elementId' in e]
        created_guids.extend(wall_guids)
        print(f"  -> Created {len(wall_guids)} walls (5 on Story 0, 5 on Story 1).")

        # 3 Columns on Story 0, 3 Columns on Story 1
        cols_data = []
        for i in range(3):
            cols_data.append({
                'coordinates': {'x': float(i * 2), 'y': 20.0, 'z': 0.0},
                'height': 3.0,
                'floorIndex': 0
            })
            cols_data.append({
                'coordinates': {'x': float(i * 2 + 10), 'y': 20.0, 'z': 3.0},
                'height': 3.0,
                'floorIndex': 1
            })

        col_res = aclib.RunTapirCommand('CreateColumns', {'columnsData': cols_data}, debug=False)
        col_guids = [e['elementId'] for e in col_res.get('elements', []) if 'elementId' in e]
        created_guids.extend(col_guids)
        print(f"  -> Created {len(col_guids)} columns (3 on Story 0, 3 on Story 1).")

        total_test_elements = len(created_guids)
        print(f"  -> Total test elements created: {total_test_elements}")

        # ── 2. BENCHMARK: Baseline (Unfiltered) vs Server-Side Projected ──
        print("\n[STEP 2] Payload & Performance Benchmark")
        
        # Query all elements in project
        all_elems_res = aclib.RunTapirCommand('GetAllElements', {}, debug=False)
        all_elements = all_elems_res.get('elements', [])
        print(f"  Total elements in active Archicad database: {len(all_elements)}")

        # A: Old-style unfiltered dump (calculates full details, all polygons, materials, pens)
        t0 = time.perf_counter()
        raw_res = aclib.RunTapirCommand('GetDetailsOfElements', {'elements': all_elements}, debug=False)
        t_baseline = (time.perf_counter() - t0) * 1000
        baseline_bytes = len(json.dumps(raw_res))

        # B: New-style projected query (implicit database scan, returns only requested fields)
        t0 = time.perf_counter()
        proj_res = aclib.RunTapirCommand('GetDetailsOfElements', {
            'fields': ['elementId', 'id', 'type', 'floorIndex', 'layerIndex']
        }, debug=False)
        t_projected = (time.perf_counter() - t0) * 1000
        projected_bytes = len(json.dumps(proj_res))

        reduction_pct = (1.0 - (projected_bytes / baseline_bytes)) * 100.0

        print("\n  ================================================================")
        print("  BENCHMARK METRICS (FULL PROJECT SCAN)")
        print("  ================================================================")
        print(f"  Baseline (Unfiltered Dump) : {baseline_bytes:8,d} bytes  |  {t_baseline:6.2f} ms")
        print(f"  Projected (Light Fields)   : {projected_bytes:8,d} bytes  |  {t_projected:6.2f} ms")
        print(f"  Payload Reduction          : {reduction_pct:7.2f}%")
        print("  ================================================================")
        assert reduction_pct > 80.0, f"Expected >80% reduction, got {reduction_pct:.2f}%"

        # ── 3. TEST: Server-Side Single Type Filter ──
        print("\n[STEP 3] Testing Filter by Type: ['Wall']")
        t0 = time.perf_counter()
        walls_only = aclib.RunTapirCommand('GetDetailsOfElements', {
            'filter': {'types': ['Wall']},
            'fields': ['elementId', 'type', 'floorIndex', 'id']
        }, debug=False)['detailsOfElements']
        t_filter = (time.perf_counter() - t0) * 1000

        wall_count = len(walls_only)
        print(f"  Returned {wall_count} elements in {t_filter:.2f} ms.")
        assert wall_count == 10, f"Expected 10 walls, got {wall_count}"
        for w in walls_only:
            assert w['type'] == 'Wall', f"Unexpected non-wall element: {w}"
            assert 'elementId' in w, "elementId missing in projected output"
        print("  [PASS] Single type filter accurate: 10/10 walls.")

        # ── 4. TEST: Server-Side Multi-Type Filter ──
        print("\n[STEP 4] Testing Filter by Multiple Types: ['Wall', 'Column']")
        walls_and_cols = aclib.RunTapirCommand('GetDetailsOfElements', {
            'filter': {'types': ['Wall', 'Column']},
            'fields': ['elementId', 'type', 'floorIndex']
        }, debug=False)['detailsOfElements']
        
        types_found = {e['type'] for e in walls_and_cols}
        print(f"  Returned {len(walls_and_cols)} elements with types: {types_found}")
        assert len(walls_and_cols) == 16, f"Expected 16 elements (10 walls + 6 cols), got {len(walls_and_cols)}"
        assert types_found == {'Wall', 'Column'}, f"Unexpected types: {types_found}"
        print("  [PASS] Multi-type filter accurate: 16/16 elements.")

        # ── 5. TEST: Combined Filter (Type + Floor Index) ──
        print("\n[STEP 5] Testing Combined Filter: Type='Wall' AND FloorIndex=1")
        story1_walls = aclib.RunTapirCommand('GetDetailsOfElements', {
            'filter': {
                'types': ['Wall'],
                'floorIndices': [1]
            },
            'fields': ['elementId', 'type', 'floorIndex', 'id']
        }, debug=False)['detailsOfElements']

        print(f"  Returned {len(story1_walls)} walls on Story 1.")
        assert len(story1_walls) == 5, f"Expected 5 walls on Story 1, got {len(story1_walls)}"
        for w in story1_walls:
            assert w['type'] == 'Wall' and w['floorIndex'] == 1
        print("  [PASS] Combined Type + Floor filter accurate: 5/5 walls.")

        # ── 6. TEST: Combined Filter (Type + FloorIndex=0) ──
        print("\n[STEP 6] Testing Combined Filter: Type='Column' AND FloorIndex=0")
        story0_cols = aclib.RunTapirCommand('GetDetailsOfElements', {
            'filter': {
                'types': ['Column'],
                'floorIndices': [0]
            },
            'fields': ['elementId', 'type', 'floorIndex']
        }, debug=False)['detailsOfElements']

        print(f"  Returned {len(story0_cols)} columns on Story 0.")
        assert len(story0_cols) == 3, f"Expected 3 columns on Story 0, got {len(story0_cols)}"
        for c in story0_cols:
            assert c['type'] == 'Column' and c['floorIndex'] == 0
        print("  [PASS] Combined Type + Floor filter accurate: 3/3 columns.")

        # ── 7. TEST: Explicit Elements List with Server-Side Filter ──
        print("\n[STEP 7] Testing Explicit Elements list + Filter")
        # Pass all 16 created element GUIDs explicitly, but filter for Column only:
        explicit_input = [{'elementId': g} for g in created_guids]
        filtered_explicit = aclib.RunTapirCommand('GetDetailsOfElements', {
            'elements': explicit_input,
            'filter': {'types': ['Column']},
            'fields': ['elementId', 'type']
        }, debug=False)['detailsOfElements']

        print(f"  Passed 16 mixed elements; filter kept {len(filtered_explicit)} columns.")
        assert len(filtered_explicit) == 6, f"Expected 6 columns, got {len(filtered_explicit)}"
        for c in filtered_explicit:
            assert c['type'] == 'Column'
        print("  [PASS] Explicit elements + filter accurate: 6/6 columns.")

        print("\n" + "=" * 70)
        print(" ALL 6 EXTENSIVE TEST SUITES PASSED PERFECTLY!")
        print("=" * 70)

    finally:
        # ── 8. TEARDOWN: Clean up created elements ──
        print("\n[TEARDOWN] Cleaning up created test elements...")
        if created_guids:
            del_res = aclib.RunTapirCommand('DeleteElements', {
                'elements': [{'elementId': g} for g in created_guids]
            }, debug=False)
            print(f"  Successfully deleted {len(created_guids)} test elements.")

if __name__ == '__main__':
    run_test()
