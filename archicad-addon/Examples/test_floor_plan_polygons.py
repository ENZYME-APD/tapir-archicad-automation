"""
test_floor_plan_polygons.py
Teste GetDetailsOfElements.floorPlanPolygons sur des murs en T créés à la volée.

Scénario :
  1. Récupère les deux premiers étages disponibles dans le fichier
  2. Sur chaque étage, crée 3 murs formant un T (mur horizontal + mur vertical)
  3. Appelle GetDetailsOfElements pour récupérer floorPlanPolygons
  4. Vérifie que chaque mur retourne au moins un polygone non vide
  5. Crée une polyligne par mur (union des pièces) pour visualisation
  6. Supprime les murs et polylignes créés (nettoyage)
  7. Affiche PASS / FAIL
"""
import sys, math
sys.path.insert(0, r"D:\ONEDRIVE\Documents\CODE PLUGINS\tapir-archicad-automation\archicad-addon\Examples")
import aclib

def run(cmd, p):
    return aclib.RunTapirCommand(cmd, p, debug=False)

# ── Helpers ───────────────────────────────────────────────────────────────────
def coord(x, y):
    return {"x": x, "y": y}

def union_polygons(raw_polys):
    """Retourne le polygone union (extérieur) à partir d'une liste de raw polygons."""
    from shapely.geometry import Polygon
    from shapely.ops import unary_union
    pieces = []
    for p in raw_polys:
        pts = [(c["x"], c["y"]) for c in p.get("coordinates", [])]
        if len(pts) < 3:
            continue
        try:
            sp = Polygon(pts)
            if not sp.is_valid:
                sp = sp.buffer(0)
            if sp.area > 1e-9:
                pieces.append(sp)
        except Exception:
            continue
    if not pieces:
        return None
    fp = unary_union(pieces)
    if fp.is_empty:
        return None
    if fp.geom_type == "MultiPolygon":
        fp = max(fp.geoms, key=lambda g: g.area)
    from shapely.geometry import Polygon as SPoly
    return SPoly(fp.exterior)

def create_t_walls(floor_index, size=3.0, thickness=0.3):
    """Crée 3 murs en T sur l'étage floor_index. Retourne les elementIds créés."""
    half_t = thickness / 2
    walls_data = [
        {
            "begCoordinate": coord(-size, 0.0),
            "endCoordinate": coord( size, 0.0),
            "floorIndex": floor_index,
            "zCoordinate": 0.0,
            "height": 3.0,
            "thickness": thickness,
        },
        {
            "begCoordinate": coord(-half_t, 0.0),
            "endCoordinate": coord(-half_t, -size),
            "floorIndex": floor_index,
            "zCoordinate": 0.0,
            "height": 3.0,
            "thickness": thickness,
        },
        {
            "begCoordinate": coord( half_t, 0.0),
            "endCoordinate": coord( half_t, -size),
            "floorIndex": floor_index,
            "zCoordinate": 0.0,
            "height": 3.0,
            "thickness": thickness,
        },
    ]
    result = run("CreateWalls", {"wallsData": walls_data})
    return [el["elementId"] for el in (result or {}).get("elements", [])]

def test_floor(story):
    floor_index = story["index"]
    label       = f"Étage {floor_index} (z={story['level']:.2f}m, nom={story.get('name', '?')!r})"
    print(f"\n── {label} ──────────────────────────────")

    # 1. Créer les murs en T directement sur cet étage (sans changer de vue)
    wall_ids = create_t_walls(floor_index)
    if len(wall_ids) != 3:
        print(f"  FAIL : CreateWalls a retourné {len(wall_ids)} mur(s) au lieu de 3")
        return [], []

    elements = [{"elementId": wid} for wid in wall_ids]

    # 2. GetDetailsOfElements → floorPlanPolygons
    dets = run("GetDetailsOfElements", {"elements": elements}).get("detailsOfElements", [])

    created_polylines = []
    all_pass = True

    for i, (wid, det) in enumerate(zip(wall_ids, dets)):
        guid = wid["guid"]
        raw_polys = det.get("floorPlanPolygons", [])
        poly = union_polygons(raw_polys)

        if poly is None or poly.area < 1e-6:
            print(f"  FAIL mur {i+1} ({guid[:8]}) : floorPlanPolygons vide ou absent")
            all_pass = False
            continue

        print(f"  OK   mur {i+1} ({guid[:8]}) : {len(raw_polys)} pièce(s) → {poly.area:.4f} m²")

        # 3. Créer une polyligne de contour sur le même étage
        coords_list = [{"x": x, "y": y} for x, y in poly.exterior.coords]
        pl_result = run("CreatePolylines", {"polylinesData": [{"coordinates": coords_list, "floorInd": floor_index}]})
        pl_els = (pl_result or {}).get("elements", [])
        if pl_els:
            created_polylines.append(pl_els[0]["elementId"])

    if all_pass:
        print(f"  PASS {label} — {len(created_polylines)} polyligne(s) créée(s)")
    else:
        print(f"  FAIL {label}")

    return [wid for wid in wall_ids], created_polylines

# ── Main ──────────────────────────────────────────────────────────────────────
print(f"ArchiCAD : {run('GetArchicadLocation', {}).get('archicadLocation', '?')}")

stories = run("GetStories", {}).get("stories", [])
if len(stories) < 2:
    print(f"SKIP : seulement {len(stories)} étage(s) disponible(s), il en faut 2")
    sys.exit(0)

# Prendre les 2 premiers étages
test_stories = sorted(stories, key=lambda s: s["index"])[:2]

all_walls     = []
all_polylines = []

for story in test_stories:
    walls, polylines = test_floor(story)
    all_walls.extend(walls)
    all_polylines.extend(polylines)

# ── Nettoyage ─────────────────────────────────────────────────────────────────
# to_delete = [{"elementId": wid} for wid in all_walls] + \
#             [{"elementId": pid} for pid in all_polylines]
# if to_delete:
#     run("DeleteElements", {"elements": to_delete})
#     print(f"  {len(all_walls)} mur(s) + {len(all_polylines)} polyligne(s) supprimés")
print(f"\n{len(all_walls)} mur(s) + {len(all_polylines)} polyligne(s) laissés dans le fichier")
